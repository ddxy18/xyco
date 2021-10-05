#include "utils/result.h"

#include <gtest/gtest.h>

class ResultTest : public ::testing::Test {
 protected:
  void SetUp() override {
    no_void_ok_ = Result<int, int>::ok(1);
    no_void_err_ = Result<int, int>::err(1);
    void_t_ok_ = Result<void, int>::ok();
    void_t_err_ = Result<void, int>::err(1);
    void_e_ok_ = Result<int, void>::ok(1);
    void_e_err_ = Result<int, void>::err();
    void_t_void_e_ok_ = Result<void, void>::ok();
    void_t_void_e_err_ = Result<void, void>::err();
  }

 public:
  Result<int, int> no_void_ok_;
  Result<int, int> no_void_err_;
  Result<void, int> void_t_ok_;
  Result<void, int> void_t_err_;
  Result<int, void> void_e_ok_;
  Result<int, void> void_e_err_;
  Result<void, void> void_t_void_e_ok_;
  Result<void, void> void_t_void_e_err_;
};

TEST_F(ResultTest, Isok) {
  ASSERT_TRUE(no_void_ok_.is_ok());
  ASSERT_TRUE(no_void_err_.is_err());
  ASSERT_TRUE(void_t_ok_.is_ok());
  ASSERT_TRUE(void_t_err_.is_err());
  ASSERT_TRUE(void_e_ok_.is_ok());
  ASSERT_TRUE(void_e_err_.is_err());
  ASSERT_TRUE(void_t_void_e_ok_.is_ok());
  ASSERT_TRUE(void_t_void_e_err_.is_err());
}

TEST_F(ResultTest, Unwrap) {
  ASSERT_EQ(no_void_ok_.unwrap(), 1);
  ASSERT_EQ(no_void_err_.unwrap_err(), 1);
  ASSERT_NO_THROW(void_t_ok_.unwrap());
  ASSERT_EQ(void_t_err_.unwrap_err(), 1);
  ASSERT_EQ(void_e_ok_.unwrap(), 1);
  ASSERT_NO_THROW(void_e_err_.unwrap_err());
  ASSERT_NO_THROW(void_t_void_e_ok_.unwrap());
  ASSERT_NO_THROW(void_t_void_e_err_.unwrap_err());
}

TEST_F(ResultTest, UnwrapOrElse) {
  auto int_to_int_f = [](auto n) { return -1; };
  auto int_to_void_f = [](auto n) {};
  auto void_to_int_f = []() { return -1; };
  auto void_to_void_f = []() {};

  ASSERT_EQ(no_void_ok_.unwrap_or_else(int_to_int_f), 1);
  ASSERT_EQ(no_void_err_.unwrap_or_else(int_to_int_f), -1);
  void_t_ok_.unwrap_or_else(int_to_void_f);
  void_t_err_.unwrap_or_else(int_to_void_f);
  ASSERT_EQ(void_e_ok_.unwrap_or_else(void_to_int_f), 1);
  ASSERT_EQ(void_e_err_.unwrap_or_else(void_to_int_f), -1);
  void_t_void_e_ok_.unwrap_or_else(void_to_void_f);
  void_t_void_e_err_.unwrap_or_else(void_to_void_f);
}

TEST_F(ResultTest, Map) {
  auto int2str = [](auto n) { return "a"; };
  auto void2str = []() { return "a"; };

  ASSERT_EQ(no_void_ok_.map(int2str).unwrap(), "a");
  ASSERT_EQ(no_void_err_.map(int2str).unwrap_err(), 1);
  ASSERT_EQ(void_t_ok_.map(void2str).unwrap(), "a");
  ASSERT_EQ(void_t_err_.map(void2str).unwrap_err(), 1);
  ASSERT_EQ(void_e_ok_.map(int2str).unwrap(), "a");
  ASSERT_NO_THROW(void_e_err_.map(int2str).unwrap_err());
  ASSERT_EQ(void_t_void_e_ok_.map(void2str).unwrap(), "a");
  ASSERT_NO_THROW(void_t_void_e_err_.map(void2str).unwrap_err());
}

TEST_F(ResultTest, MapToVoid) {
  auto int2void = [](auto n) {};

  ASSERT_NO_THROW(no_void_ok_.map(int2void).unwrap());
  ASSERT_EQ(no_void_err_.map(int2void).unwrap_err(), 1);
}

TEST_F(ResultTest, MapErr) {
  auto int2str = [](auto n) { return "a"; };
  auto void2str = []() { return "a"; };

  ASSERT_EQ(no_void_ok_.map_err(int2str).unwrap(), 1);
  ASSERT_EQ(no_void_err_.map_err(int2str).unwrap_err(), "a");
  ASSERT_NO_THROW(void_t_ok_.map_err(int2str).unwrap());
  ASSERT_EQ(void_t_err_.map_err(int2str).unwrap_err(), "a");
  ASSERT_EQ(void_e_ok_.map_err(void2str).unwrap(), 1);
  ASSERT_EQ(void_e_err_.map_err(void2str).unwrap_err(), "a");
  ASSERT_NO_THROW(void_t_void_e_ok_.map_err(void2str).unwrap());
  ASSERT_EQ(void_t_void_e_err_.map_err(void2str).unwrap_err(), "a");
}

TEST_F(ResultTest, MapErrToVoid) {
  auto int2void = [](auto n) {};

  ASSERT_EQ(no_void_ok_.map_err(int2void).unwrap(), 1);
  ASSERT_NO_THROW(no_void_err_.map_err(int2void).unwrap_err());
}

TEST_F(ResultTest, Pointer) {
  auto value = std::make_unique<int>(1);
  auto result = Result<int *, int>::ok(value.get());
  ASSERT_EQ(*result.unwrap(), 1);
}