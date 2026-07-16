/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/stmt/update_stmt.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

UpdateStmt::UpdateStmt(Table *table,
    std::vector<const FieldMeta *> field_metas,
    std::vector<Value> values,
    FilterStmt *filter_stmt)
    : table_(table), field_metas_(std::move(field_metas)), values_(std::move(values)), filter_stmt_(filter_stmt)
{}

UpdateStmt::~UpdateStmt()
{
  delete filter_stmt_;
  filter_stmt_ = nullptr;
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  stmt = nullptr;

  const char *table_name = update.relation_name.c_str();
  if (db == nullptr || table_name == nullptr) {
    LOG_WARN("invalid argument. db=%p, table_name=%p", db, table_name);
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  std::unordered_map<std::string, Table *> table_map;
  table_map.emplace(table_name, table);

  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(db,
      table,
      &table_map,
      update.conditions.data(),
      static_cast<int>(update.conditions.size()),
      filter_stmt);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to create filter stmt. rc=%s", strrc(rc));
    return rc;
  }

  std::vector<UpdateValueSqlNode> assignments = update.values;
  if (assignments.empty()) {
    UpdateValueSqlNode assignment;
    assignment.attribute_name  = update.attribute_name;
    assignment.value           = update.value;
    assignment.value_subquery  = update.value_subquery;
    assignments.emplace_back(std::move(assignment));
  }

  std::vector<const FieldMeta *> field_metas;
  std::vector<Value>             values;
  field_metas.reserve(assignments.size());
  values.reserve(assignments.size());

  for (const UpdateValueSqlNode &assignment : assignments) {
    const FieldMeta *field_meta = table->table_meta().field(assignment.attribute_name.c_str());
    if (field_meta == nullptr) {
      delete filter_stmt;
      LOG_WARN("no such field in table. table=%s, field=%s", table_name, assignment.attribute_name.c_str());
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }

    Value value = assignment.value;
    if (assignment.value_subquery) {
      std::vector<Value> subquery_values;
      rc = FilterStmt::eval_simple_subquery(db, *assignment.value_subquery, subquery_values);
      if (OB_FAIL(rc)) {
        delete filter_stmt;
        LOG_WARN("failed to evaluate update subquery. table=%s, rc=%s", table_name, strrc(rc));
        return rc;
      }
      if (subquery_values.size() > 1) {
        delete filter_stmt;
        LOG_WARN("update subquery should return at most one value. table=%s, size=%d",
            table_name,
            static_cast<int>(subquery_values.size()));
        return RC::INVALID_ARGUMENT;
      }
      if (subquery_values.empty()) {
        value.set_null();
      } else {
        value = subquery_values.front();
      }
    }
    if (value.attr_type() != field_meta->type()) {
      Value cast_value;
      rc = Value::cast_to(value, field_meta->type(), cast_value);
      if (OB_FAIL(rc)) {
        delete filter_stmt;
        LOG_WARN("failed to cast update value. table=%s, field=%s, rc=%s", table_name, field_meta->name(), strrc(rc));
        return rc;
      }
      value = cast_value;
    }

    field_metas.emplace_back(field_meta);
    values.emplace_back(std::move(value));
  }

  stmt = new UpdateStmt(table, std::move(field_metas), std::move(values), filter_stmt);
  return RC::SUCCESS;
}
