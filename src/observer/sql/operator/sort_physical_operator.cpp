/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/sort_physical_operator.h"

#include <algorithm>

#include "common/log/log.h"

SortPhysicalOperator::SortPhysicalOperator(std::vector<std::unique_ptr<Expression>> &&expressions, std::vector<bool> &&asc)
    : expressions_(std::move(expressions)), asc_(std::move(asc))
{}

RC SortPhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  RC rc = children_[0]->open(trx);
  if (OB_FAIL(rc)) {
    return rc;
  }

  tuples_.clear();
  tuple_index_ = -1;
  schema_      = TupleSchema();

  rc = children_[0]->tuple_schema(schema_);
  if (OB_FAIL(rc)) {
    return rc;
  }

  while ((rc = children_[0]->next()) == RC::SUCCESS) {
    Tuple *current_tuple = children_[0]->current_tuple();
    SortItem item;

    rc = ValueListTuple::make(*current_tuple, item.tuple);
    if (OB_FAIL(rc)) {
      return rc;
    }

    item.keys.reserve(expressions_.size());
    for (const std::unique_ptr<Expression> &expression : expressions_) {
      Value key;
      rc = expression->get_value(*current_tuple, key);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to evaluate order by expression. rc=%s", strrc(rc));
        return rc;
      }
      item.keys.emplace_back(key);
    }

    tuples_.push_back(std::move(item));
  }
  if (rc != RC::RECORD_EOF) {
    return rc;
  }

  std::sort(tuples_.begin(), tuples_.end(), [this](const SortItem &lhs, const SortItem &rhs) {
    for (size_t i = 0; i < expressions_.size(); i++) {
      int cmp = lhs.keys[i].compare(rhs.keys[i]);
      if (cmp != 0) {
        bool asc = i >= asc_.size() ? true : asc_[i];
        return asc ? cmp < 0 : cmp > 0;
      }
    }
    return false;
  });
  return RC::SUCCESS;
}

RC SortPhysicalOperator::next()
{
  if (tuple_index_ + 1 >= static_cast<int>(tuples_.size())) {
    return RC::RECORD_EOF;
  }
  tuple_index_++;
  return RC::SUCCESS;
}

RC SortPhysicalOperator::close()
{
  tuples_.clear();
  tuple_index_ = -1;
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}

Tuple *SortPhysicalOperator::current_tuple()
{
  if (tuple_index_ < 0 || tuple_index_ >= static_cast<int>(tuples_.size())) {
    return nullptr;
  }
  return &tuples_[tuple_index_].tuple;
}

RC SortPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  schema = schema_;
  return RC::SUCCESS;
}
