/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <vector>

#include "common/rc.h"
#include "sql/stmt/stmt.h"

class Table;
class FieldMeta;
class FilterStmt;

class UpdateStmt : public Stmt
{
public:
  UpdateStmt(Table *table,
      std::vector<const FieldMeta *> field_metas,
      std::vector<Value> values,
      FilterStmt *filter_stmt);
  ~UpdateStmt() override;

  StmtType type() const override { return StmtType::UPDATE; }

  Table *table() const { return table_; }
  const std::vector<const FieldMeta *> &field_metas() const { return field_metas_; }
  const std::vector<Value> &values() const { return values_; }
  const FieldMeta *field_meta() const { return field_metas_.front(); }
  const Value &value() const { return values_.front(); }
  FilterStmt *filter_stmt() const { return filter_stmt_; }

  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

private:
  Table *table_ = nullptr;
  std::vector<const FieldMeta *> field_metas_;
  std::vector<Value> values_;
  FilterStmt *filter_stmt_ = nullptr;
};
