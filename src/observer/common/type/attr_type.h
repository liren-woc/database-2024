#pragma once

enum class AttrType
{
  UNDEFINED,
  CHARS,
  INTS,
  FLOATS,
  DATES,
  NULLS,
  VECTORS,
  BOOLEANS,
  MAXTYPE,
};

const char *attr_type_to_string(AttrType type);
AttrType    attr_type_from_string(const char *s);
