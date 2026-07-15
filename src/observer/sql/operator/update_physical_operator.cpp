/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/update_physical_operator.h"

#include <algorithm>
#include <cstring>
#include "common/log/log.h"
#include "storage/table/table.h"

RC UpdatePhysicalOperator::open(Trx *trx)
{
  (void)trx;
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  auto &child = children_[0];
  RC rc = child->open(trx);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

  std::vector<Record> records;
  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (tuple == nullptr) {
      child->close();
      return RC::INTERNAL;
    }
    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    records.emplace_back(row_tuple->record());
  }
  child->close();
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to iterate records for update. rc=%s", strrc(rc));
    return rc;
  }

  for (Record &record : records) {
    Record new_record(record);

    if (value_.is_null()) {
      if (!field_meta_->nullable()) {
        return RC::INVALID_ARGUMENT;
      }
      if (field_meta_->null_offset() >= 0) {
        new_record.data()[field_meta_->null_offset()] = 1;
      }
      memset(new_record.data() + field_meta_->offset(), 0, field_meta_->len());
    } else {
      if (field_meta_->null_offset() >= 0) {
        new_record.data()[field_meta_->null_offset()] = 0;
      }
      memset(new_record.data() + field_meta_->offset(), 0, field_meta_->len());

      size_t copy_len = field_meta_->len();
      if (field_meta_->type() == AttrType::CHARS || field_meta_->type() == AttrType::DATES) {
        copy_len = std::min(copy_len, static_cast<size_t>(value_.length() + 1));
      } else {
        copy_len = std::min(copy_len, static_cast<size_t>(value_.length()));
      }
      memcpy(new_record.data() + field_meta_->offset(), value_.data(), copy_len);
    }

    rc = table_->update_record(record, new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to update record. table=%s, rc=%s", table_->name(), strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}
