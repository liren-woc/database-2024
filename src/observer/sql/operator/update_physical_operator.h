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

#include <utility>

#include "sql/operator/physical_operator.h"

class Table;
class FieldMeta;

class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, const FieldMeta *field_meta, Value value)
      : table_(table), field_meta_(field_meta), value_(std::move(value))
  {}

  PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }

  RC open(Trx *trx) override;
  RC next() override { return RC::RECORD_EOF; }
  RC close() override { return RC::SUCCESS; }
  Tuple *current_tuple() override { return nullptr; }

private:
  Table *table_ = nullptr;
  const FieldMeta *field_meta_ = nullptr;
  Value value_;
};
