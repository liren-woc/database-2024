/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/insert_physical_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

using namespace std;

InsertPhysicalOperator::InsertPhysicalOperator(Table *table, vector<vector<Value>> &&value_groups)
    : table_(table), value_groups_(std::move(value_groups))
{}

RC InsertPhysicalOperator::open(Trx *trx)
{
  vector<Record> inserted_records;
  inserted_records.reserve(value_groups_.size());

  RC rc = RC::SUCCESS;
  for (vector<Value> &values : value_groups_) {
    Record record;
    rc = table_->make_record(static_cast<int>(values.size()), values.data(), record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to make record. rc=%s", strrc(rc));
      break;
    }

    rc = trx->insert_record(table_, record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert record by transaction. rc=%s", strrc(rc));
      break;
    }

    inserted_records.emplace_back(std::move(record));
  }

  if (OB_FAIL(rc)) {
    for (auto it = inserted_records.rbegin(); it != inserted_records.rend(); ++it) {
      RC rollback_rc = trx->delete_record(table_, *it);
      if (OB_FAIL(rollback_rc)) {
        LOG_PANIC("failed to rollback inserted record. table=%s, rid=%s, rc=%s",
            table_->name(), it->rid().to_string().c_str(), strrc(rollback_rc));
      }
    }
  }
  return rc;
}

RC InsertPhysicalOperator::next() { return RC::RECORD_EOF; }

RC InsertPhysicalOperator::close() { return RC::SUCCESS; }
