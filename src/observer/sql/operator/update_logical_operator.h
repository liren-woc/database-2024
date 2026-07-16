/* Copyright (c) OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <utility>
#include <vector>

#include "sql/operator/logical_operator.h"

class Table;
class FieldMeta;

class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, std::vector<const FieldMeta *> field_metas, std::vector<Value> values)
      : table_(table), field_metas_(std::move(field_metas)), values_(std::move(values))
  {}

  LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }
  Table *table() const { return table_; }
  const std::vector<const FieldMeta *> &field_metas() const { return field_metas_; }
  const std::vector<Value> &values() const { return values_; }
  const FieldMeta *field_meta() const { return field_metas_.front(); }
  const Value &value() const { return values_.front(); }

private:
  Table *table_ = nullptr;
  std::vector<const FieldMeta *> field_metas_;
  std::vector<Value> values_;
};
