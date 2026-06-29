/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/type/date_type.h"

#include <cstdio>

#include "common/lang/comparator.h"
#include "common/log/log.h"
#include "common/value.h"

int DateType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::DATES && right.attr_type() == AttrType::DATES, "invalid type");
  return common::compare_string(
      (void *)left.value_.pointer_value_, left.length_, (void *)right.value_.pointer_value_, right.length_);
}

RC DateType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
    case AttrType::DATES: {
      result.set_date(val.get_string().c_str());
      return RC::SUCCESS;
    }
    case AttrType::CHARS: {
      result.set_string(val.get_string().c_str());
      return RC::SUCCESS;
    }
    default: return RC::UNIMPLEMENTED;
  }
}

RC DateType::set_value_from_str(Value &val, const string &data) const
{
  string normalized;
  if (!normalize(data, normalized)) {
    return RC::INVALID_ARGUMENT;
  }
  val.set_date(normalized.c_str());
  return RC::SUCCESS;
}

int DateType::cast_cost(AttrType type)
{
  if (type == AttrType::DATES) {
    return 0;
  }
  if (type == AttrType::CHARS) {
    return 1;
  }
  return INT32_MAX;
}

RC DateType::to_string(const Value &val, string &result) const
{
  result = val.value_.pointer_value_ == nullptr ? "" : val.value_.pointer_value_;
  return RC::SUCCESS;
}

static bool is_leap_year(int year)
{
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

bool DateType::normalize(const string &input, string &output)
{
  int  year = 0;
  int  month = 0;
  int  day = 0;
  char tail = 0;
  if (sscanf(input.c_str(), "%d-%d-%d%c", &year, &month, &day, &tail) != 3) {
    return false;
  }

  if (year < 0 || year > 9999 || month < 1 || month > 12 || day < 1) {
    return false;
  }

  static const int month_days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int max_day = month_days[month];
  if (month == 2 && is_leap_year(year)) {
    max_day = 29;
  }
  if (day > max_day) {
    return false;
  }

  char buffer[11];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);
  output = buffer;
  return true;
}
