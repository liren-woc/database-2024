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
  if (field_metas_.size() != values_.size()) {
    return RC::INTERNAL;
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

    Record record;
    rc = record.copy_data(row_tuple->record().data(), row_tuple->record().len());
    if (OB_FAIL(rc)) {
      child->close();
      LOG_WARN("failed to copy record before update. rc=%s", strrc(rc));
      return rc;
    }
    record.set_rid(row_tuple->record().rid());
    records.emplace_back(std::move(record));
  }
  child->close();
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to iterate records for update. rc=%s", strrc(rc));
    return rc;
  }

  for (Record &record : records) {
    Record new_record(record);

    for (size_t i = 0; i < field_metas_.size(); i++) {
      const FieldMeta *field_meta = field_metas_[i];
      const Value     &value      = values_[i];
      if (value.is_null()) {
        if (!field_meta->nullable()) {
          return RC::INVALID_ARGUMENT;
        }
        if (field_meta->null_offset() >= 0) {
          new_record.data()[field_meta->null_offset()] = 1;
        }
        memset(new_record.data() + field_meta->offset(), 0, field_meta->len());
      } else {
        if (field_meta->null_offset() >= 0) {
          new_record.data()[field_meta->null_offset()] = 0;
        }
        memset(new_record.data() + field_meta->offset(), 0, field_meta->len());

        size_t copy_len = field_meta->len();
        if (field_meta->type() == AttrType::CHARS || field_meta->type() == AttrType::DATES) {
          copy_len = std::min(copy_len, static_cast<size_t>(value.length() + 1));
        } else {
          copy_len = std::min(copy_len, static_cast<size_t>(value.length()));
        }
        memcpy(new_record.data() + field_meta->offset(), value.data(), copy_len);
      }
    }

    rc = table_->update_record(record, new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to update record. table=%s, rc=%s", table_->name(), strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}
