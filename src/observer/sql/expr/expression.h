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
// Created by Wangyunlai on 2022/07/05.
//

#pragma once

#include <memory>
#include <string>

#include "common/value.h"
#include "storage/field/field.h"
#include "sql/expr/aggregator.h"
#include "storage/common/chunk.h"

class Tuple;

/**
 * @defgroup Expression
 * @brief 琛ㄨ揪寮?
 */

/**
 * @brief 琛ㄨ揪寮忕被鍨?
 * @ingroup Expression
 */
enum class ExprType
{
  NONE,
  STAR,                 ///< 鏄熷彿锛岃〃绀烘墍鏈夊瓧娈?
  UNBOUND_FIELD,        ///< 鏈粦瀹氱殑瀛楁锛岄渶瑕佸湪resolver闃舵瑙ｆ瀽涓篎ieldExpr
  UNBOUND_AGGREGATION,  ///< 鏈粦瀹氱殑鑱氬悎鍑芥暟锛岄渶瑕佸湪resolver闃舵瑙ｆ瀽涓篈ggregateExpr

  FIELD,        ///< 瀛楁銆傚湪瀹為檯鎵ц鏃讹紝鏍规嵁琛屾暟鎹唴瀹规彁鍙栧搴斿瓧娈电殑鍊?
  VALUE,        ///< 甯搁噺鍊?
  CAST,         ///< 闇€瑕佸仛绫诲瀷杞崲鐨勮〃杈惧紡
  COMPARISON,   ///< 闇€瑕佸仛姣旇緝鐨勮〃杈惧紡
  CONJUNCTION,  ///< 澶氫釜琛ㄨ揪寮忎娇鐢ㄥ悓涓€绉嶅叧绯?AND鎴朞R)鏉ヨ仈缁?
  ARITHMETIC,   ///< 绠楁湳杩愮畻
  AGGREGATION,  ///< 鑱氬悎杩愮畻
  IN_LIST,
};

/**
 * @brief 琛ㄨ揪寮忕殑鎶借薄鎻忚堪
 * @ingroup Expression
 * @details 鍦⊿QL鐨勫厓绱犱腑锛屼换浣曢渶瑕佸緱鍑哄€肩殑鍏冪礌閮藉彲浠ヤ娇鐢ㄨ〃杈惧紡鏉ユ弿杩?
 * 姣斿鑾峰彇鏌愪釜瀛楁鐨勫€笺€佹瘮杈冭繍绠椼€佺被鍨嬭浆鎹?
 * 褰撶劧杩樻湁涓€浜涘綋鍓嶆病鏈夊疄鐜扮殑琛ㄨ揪寮忥紝姣斿绠楁湳杩愮畻銆?
 *
 * 閫氬父琛ㄨ揪寮忕殑鍊硷紝鏄湪鐪熷疄鐨勭畻瀛愯繍绠楄繃绋嬩腑锛屾嬁鍒板叿浣撶殑tuple鍚?
 * 鎵嶈兘璁＄畻鍑烘潵鐪熷疄鐨勫€笺€備絾鏄湁浜涜〃杈惧紡鍙兘灏辫〃绀烘煇涓€涓浐瀹氱殑
 * 鍊硷紝姣斿ValueExpr銆?
 *
 * TODO 鍖哄垎unbound鍜宐ound鐨勮〃杈惧紡
 */
class Expression
{
public:
  Expression()          = default;
  virtual ~Expression() = default;

  /**
   * @brief 鍒ゆ柇涓や釜琛ㄨ揪寮忔槸鍚︾浉绛?
   */
  virtual bool equal(const Expression &other) const { return false; }
  /**
   * @brief 鏍规嵁鍏蜂綋鐨則uple锛屾潵璁＄畻褰撳墠琛ㄨ揪寮忕殑鍊笺€倀uple鏈夊彲鑳芥槸涓€涓叿浣撴煇涓〃鐨勮鏁版嵁
   */
  virtual RC get_value(const Tuple &tuple, Value &value) const = 0;

  /**
   * @brief 鍦ㄦ病鏈夊疄闄呰繍琛岀殑鎯呭喌涓嬶紝涔熷氨鏄棤娉曡幏鍙杢uple鐨勬儏鍐典笅锛屽皾璇曡幏鍙栬〃杈惧紡鐨勫€?
   * @details 鏈変簺琛ㄨ揪寮忕殑鍊兼槸鍥哄畾鐨勶紝姣斿ValueExpr锛岃繖绉嶆儏鍐典笅鍙互鐩存帴鑾峰彇鍊?
   */
  virtual RC try_get_value(Value &value) const { return RC::UNIMPLEMENTED; }

  /**
   * @brief 浠?`chunk` 涓幏鍙栬〃杈惧紡鐨勮绠楃粨鏋?`column`
   */
  virtual RC get_column(Chunk &chunk, Column &column) { return RC::UNIMPLEMENTED; }

  /**
   * @brief 琛ㄨ揪寮忕殑绫诲瀷
   * 鍙互鏍规嵁琛ㄨ揪寮忕被鍨嬫潵杞崲涓哄叿浣撶殑瀛愮被
   */
  virtual ExprType type() const = 0;

  /**
   * @brief 琛ㄨ揪寮忓€肩殑绫诲瀷
   * @details 涓€涓〃杈惧紡杩愮畻鍑虹粨鏋滃悗锛屽彧鏈変竴涓€?
   */
  virtual AttrType value_type() const = 0;

  /**
   * @brief 琛ㄨ揪寮忓€肩殑闀垮害
   */
  virtual int value_length() const { return -1; }

  /**
   * @brief 琛ㄨ揪寮忕殑鍚嶅瓧锛屾瘮濡傛槸瀛楁鍚嶇О锛屾垨鑰呯敤鎴峰湪鎵цSQL璇彞鏃惰緭鍏ョ殑鍐呭
   */
  virtual const char *name() const { return name_.c_str(); }
  virtual void        set_name(std::string name) { name_ = name; }

  /**
   * @brief 琛ㄨ揪寮忓湪涓嬪眰绠楀瓙杩斿洖鐨?chunk 涓殑浣嶇疆
   */
  virtual int  pos() const { return pos_; }
  virtual void set_pos(int pos) { pos_ = pos; }

  /**
   * @brief 鐢ㄤ簬 ComparisonExpr 鑾峰緱姣旇緝缁撴灉 `select`銆?
   */
  virtual RC eval(Chunk &chunk, std::vector<uint8_t> &select) { return RC::UNIMPLEMENTED; }

protected:
  /**
   * @brief 琛ㄨ揪寮忓湪涓嬪眰绠楀瓙杩斿洖鐨?chunk 涓殑浣嶇疆
   * @details 褰?pos_ = -1 鏃惰〃绀轰笅灞傜畻瀛愭病鏈夊湪杩斿洖鐨?chunk 涓绠楀嚭璇ヨ〃杈惧紡鐨勮绠楃粨鏋滐紝
   * 褰?pos_ >= 0鏃惰〃绀哄湪涓嬪眰绠楀瓙涓凡缁忚绠楀嚭璇ヨ〃杈惧紡鐨勫€硷紙姣斿鑱氬悎琛ㄨ揪寮忥級锛屼笖璇ヨ〃杈惧紡瀵瑰簲鐨勭粨鏋滀綅浜?
   * chunk 涓?涓嬫爣涓?pos_ 鐨勫垪涓€?
   */
  int pos_ = -1;

private:
  std::string name_;
};

class StarExpr : public Expression
{
public:
  StarExpr() : table_name_() {}
  StarExpr(const char *table_name) : table_name_(table_name) {}
  virtual ~StarExpr() = default;

  ExprType type() const override { return ExprType::STAR; }
  AttrType value_type() const override { return AttrType::UNDEFINED; }

  RC get_value(const Tuple &tuple, Value &value) const override { return RC::UNIMPLEMENTED; }  // 涓嶉渶瑕佸疄鐜?

  const char *table_name() const { return table_name_.c_str(); }

private:
  std::string table_name_;
};

class UnboundFieldExpr : public Expression
{
public:
  UnboundFieldExpr(const std::string &table_name, const std::string &field_name)
      : table_name_(table_name), field_name_(field_name)
  {}

  virtual ~UnboundFieldExpr() = default;

  ExprType type() const override { return ExprType::UNBOUND_FIELD; }
  AttrType value_type() const override { return AttrType::UNDEFINED; }

  RC get_value(const Tuple &tuple, Value &value) const override { return RC::INTERNAL; }

  const char *table_name() const { return table_name_.c_str(); }
  const char *field_name() const { return field_name_.c_str(); }

private:
  std::string table_name_;
  std::string field_name_;
};

/**
 * @brief 瀛楁琛ㄨ揪寮?
 * @ingroup Expression
 */
class FieldExpr : public Expression
{
public:
  FieldExpr() = default;
  FieldExpr(const Table *table, const FieldMeta *field) : field_(table, field) {}
  FieldExpr(const Field &field) : field_(field) {}

  virtual ~FieldExpr() = default;

  bool equal(const Expression &other) const override;

  ExprType type() const override { return ExprType::FIELD; }
  AttrType value_type() const override { return field_.attr_type(); }
  int      value_length() const override { return field_.meta()->len(); }

  Field &field() { return field_; }

  const Field &field() const { return field_; }

  const char *table_name() const { return field_.table_name(); }
  const char *field_name() const { return field_.field_name(); }

  RC get_column(Chunk &chunk, Column &column) override;

  RC get_value(const Tuple &tuple, Value &value) const override;

private:
  Field field_;
};

/**
 * @brief 甯搁噺鍊艰〃杈惧紡
 * @ingroup Expression
 */
class ValueExpr : public Expression
{
public:
  ValueExpr() = default;
  explicit ValueExpr(const Value &value) : value_(value) {}

  virtual ~ValueExpr() = default;

  bool equal(const Expression &other) const override;

  RC get_value(const Tuple &tuple, Value &value) const override;
  RC get_column(Chunk &chunk, Column &column) override;
  RC try_get_value(Value &value) const override
  {
    value = value_;
    return RC::SUCCESS;
  }

  ExprType type() const override { return ExprType::VALUE; }
  AttrType value_type() const override { return value_.attr_type(); }
  int      value_length() const override { return value_.length(); }

  void         get_value(Value &value) const { value = value_; }
  const Value &get_value() const { return value_; }

private:
  Value value_;
};

/**
 * @brief 绫诲瀷杞崲琛ㄨ揪寮?
 * @ingroup Expression
 */
class CastExpr : public Expression
{
public:
  CastExpr(std::unique_ptr<Expression> child, AttrType cast_type);
  virtual ~CastExpr();

  ExprType type() const override { return ExprType::CAST; }

  RC get_value(const Tuple &tuple, Value &value) const override;

  RC try_get_value(Value &value) const override;

  AttrType value_type() const override { return cast_type_; }

  std::unique_ptr<Expression> &child() { return child_; }

private:
  RC cast(const Value &value, Value &cast_value) const;

private:
  std::unique_ptr<Expression> child_;      ///< 浠庤繖涓〃杈惧紡杞崲
  AttrType                    cast_type_;  ///< 鎯宠杞崲鎴愯繖涓被鍨?
};

/**
 * @brief 姣旇緝琛ㄨ揪寮?
 * @ingroup Expression
 */
class ComparisonExpr : public Expression
{
public:
  ComparisonExpr(CompOp comp, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
  virtual ~ComparisonExpr();

  ExprType type() const override { return ExprType::COMPARISON; }
  RC       get_value(const Tuple &tuple, Value &value) const override;
  AttrType value_type() const override { return AttrType::BOOLEANS; }
  CompOp   comp() const { return comp_; }

  /**
   * @brief 鏍规嵁 ComparisonExpr 鑾峰緱 `select` 缁撴灉銆?
   * select 鐨勯暱搴︿笌chunk 鐨勮鏁扮浉鍚岋紝琛ㄧず姣忎竴琛屽湪ComparisonExpr 璁＄畻鍚庢槸鍚︿細琚緭鍑恒€?
   */
  RC eval(Chunk &chunk, std::vector<uint8_t> &select) override;

  std::unique_ptr<Expression> &left() { return left_; }
  std::unique_ptr<Expression> &right() { return right_; }

  /**
   * 灏濊瘯鍦ㄦ病鏈塼uple鐨勬儏鍐典笅鑾峰彇褰撳墠琛ㄨ揪寮忕殑鍊?
   * 鍦ㄤ紭鍖栫殑鏃跺€欙紝鍙兘浼氫娇鐢ㄥ埌
   */
  RC try_get_value(Value &value) const override;

  /**
   * compare the two tuple cells
   * @param value the result of comparison
   */
  RC compare_value(const Value &left, const Value &right, bool &value) const;

  template <typename T>
  RC compare_column(const Column &left, const Column &right, std::vector<uint8_t> &result) const;

private:
  CompOp                      comp_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

/**
 * @brief 鑱旂粨琛ㄨ揪寮?
 * @ingroup Expression
 * 澶氫釜琛ㄨ揪寮忎娇鐢ㄥ悓涓€绉嶅叧绯?AND鎴朞R)鏉ヨ仈缁?
 * 褰撳墠miniob浠呮湁AND鎿嶄綔
 */
class ConjunctionExpr : public Expression
{
public:
  enum class Type
  {
    AND,
    OR,
  };

public:
  ConjunctionExpr(Type type, std::vector<std::unique_ptr<Expression>> &children);
  virtual ~ConjunctionExpr() = default;

  ExprType type() const override { return ExprType::CONJUNCTION; }
  AttrType value_type() const override { return AttrType::BOOLEANS; }
  RC       get_value(const Tuple &tuple, Value &value) const override;

  Type conjunction_type() const { return conjunction_type_; }

  std::vector<std::unique_ptr<Expression>> &children() { return children_; }

private:
  Type                                     conjunction_type_;
  std::vector<std::unique_ptr<Expression>> children_;
};

/**
 * @brief 绠楁湳琛ㄨ揪寮?
 * @ingroup Expression
 */
class InExpr : public Expression
{
public:
  InExpr(std::unique_ptr<Expression> left, std::vector<Value> values, bool negative)
      : left_(std::move(left)), values_(std::move(values)), negative_(negative)
  {}

  ExprType type() const override { return ExprType::IN_LIST; }
  AttrType value_type() const override { return AttrType::BOOLEANS; }
  RC       get_value(const Tuple &tuple, Value &value) const override;

private:
  std::unique_ptr<Expression> left_;
  std::vector<Value>          values_;
  bool                        negative_ = false;
};

class ArithmeticExpr : public Expression
{
public:
  enum class Type
  {
    ADD,
    SUB,
    MUL,
    DIV,
    NEGATIVE,
  };

public:
  ArithmeticExpr(Type type, Expression *left, Expression *right);
  ArithmeticExpr(Type type, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
  virtual ~ArithmeticExpr() = default;

  bool     equal(const Expression &other) const override;
  ExprType type() const override { return ExprType::ARITHMETIC; }

  AttrType value_type() const override;
  int      value_length() const override
  {
    if (!right_) {
      return left_->value_length();
    }
    return 4;  // sizeof(float) or sizeof(int)
  };

  RC get_value(const Tuple &tuple, Value &value) const override;

  RC get_column(Chunk &chunk, Column &column) override;

  RC try_get_value(Value &value) const override;

  Type arithmetic_type() const { return arithmetic_type_; }

  std::unique_ptr<Expression> &left() { return left_; }
  std::unique_ptr<Expression> &right() { return right_; }

private:
  RC calc_value(const Value &left_value, const Value &right_value, Value &value) const;

  RC calc_column(const Column &left_column, const Column &right_column, Column &column) const;

  template <bool LEFT_CONSTANT, bool RIGHT_CONSTANT>
  RC execute_calc(const Column &left, const Column &right, Column &result, Type type, AttrType attr_type) const;

private:
  Type                        arithmetic_type_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

class UnboundAggregateExpr : public Expression
{
public:
  UnboundAggregateExpr(const char *aggregate_name, Expression *child);
  virtual ~UnboundAggregateExpr() = default;

  ExprType type() const override { return ExprType::UNBOUND_AGGREGATION; }

  const char *aggregate_name() const { return aggregate_name_.c_str(); }

  std::unique_ptr<Expression> &child() { return child_; }

  RC       get_value(const Tuple &tuple, Value &value) const override { return RC::INTERNAL; }
  AttrType value_type() const override { return child_->value_type(); }

private:
  std::string                 aggregate_name_;
  std::unique_ptr<Expression> child_;
};

class AggregateExpr : public Expression
{
public:
  enum class Type
  {
    COUNT,
    SUM,
    AVG,
    MAX,
    MIN,
  };

public:
  AggregateExpr(Type type, Expression *child);
  AggregateExpr(Type type, std::unique_ptr<Expression> child);
  virtual ~AggregateExpr() = default;

  bool equal(const Expression &other) const override;

  ExprType type() const override { return ExprType::AGGREGATION; }

  AttrType value_type() const override { return child_->value_type(); }
  int      value_length() const override { return child_->value_length(); }

  RC get_value(const Tuple &tuple, Value &value) const override;

  RC get_column(Chunk &chunk, Column &column) override;

  Type aggregate_type() const { return aggregate_type_; }

  std::unique_ptr<Expression> &child() { return child_; }

  const std::unique_ptr<Expression> &child() const { return child_; }

  std::unique_ptr<Aggregator> create_aggregator() const;

public:
  static RC type_from_string(const char *type_str, Type &type);

private:
  Type                        aggregate_type_;
  std::unique_ptr<Expression> child_;
};
