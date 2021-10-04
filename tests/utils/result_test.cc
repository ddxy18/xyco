#include "utils/result.h"

#include <gtest/gtest.h>

class ResultTest : public ::testing::Test {
 protected:
  void SetUp() override {
    no_void_ok_ = Ok<int, int>(1);
    no_void_err_ = Err<int, int>(1);
    void_t_ok_ = Ok<int>();
    void_t_err_ = Err<void, int>(1);
    void_e_ok_ = Ok<int, void>(1);
    void_e_err_ = Err<int>();
    void_t_void_e_ok_ = Ok<void>();
    void_t_void_e_err_ = Err<void>();
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

TEST_F(ResultTest, UnwrapOr) {
  ASSERT_EQ(no_void_ok_.unwrap_or(-1), 1);
  ASSERT_EQ(no_void_err_.unwrap_or(-1), -1);
  void_t_ok_.unwrap_or();
  void_t_err_.unwrap_or();
  ASSERT_EQ(void_e_ok_.unwrap_or(-1), 1);
  ASSERT_EQ(void_e_err_.unwrap_or(-1), -1);
  void_t_void_e_ok_.unwrap_or();
  void_t_void_e_err_.unwrap_or();
}

TEST_F(ResultTest, Map) {
  auto map_int_f = std::function<const char *(int)>([](auto n) { return "a"; });
  auto map_void_f = std::function<const char *(void)>([]() { return "a"; });

  ASSERT_EQ(no_void_ok_.map(map_int_f).unwrap(), "a");
  ASSERT_EQ(no_void_err_.map(map_int_f).unwrap_err(), 1);
  ASSERT_EQ(void_t_ok_.map(map_void_f).unwrap(), "a");
  ASSERT_EQ(void_t_err_.map(map_void_f).unwrap_err(), 1);
  ASSERT_EQ(void_e_ok_.map(map_int_f).unwrap(), "a");
  ASSERT_NO_THROW(void_e_err_.map(map_int_f).unwrap_err());
  ASSERT_EQ(void_t_void_e_ok_.map(map_void_f).unwrap(), "a");
  ASSERT_NO_THROW(void_t_void_e_err_.map(map_void_f).unwrap_err());
}

TEST_F(ResultTest, MapErr) {
  auto map_int_f = std::function<const char *(int)>([](auto n) { return "a"; });
  auto map_void_f = std::function<const char *(void)>([]() { return "a"; });

  ASSERT_EQ(no_void_ok_.map_err(map_int_f).unwrap(), 1);
  ASSERT_EQ(no_void_err_.map_err(map_int_f).unwrap_err(), "a");
  ASSERT_NO_THROW(void_t_ok_.map_err(map_int_f).unwrap());
  ASSERT_EQ(void_t_err_.map_err(map_int_f).unwrap_err(), "a");
  ASSERT_EQ(void_e_ok_.map_err(map_void_f).unwrap(), 1);
  ASSERT_EQ(void_e_err_.map_err(map_void_f).unwrap_err(), "a");
  ASSERT_NO_THROW(void_t_void_e_ok_.map_err(map_void_f).unwrap());
  ASSERT_EQ(void_t_void_e_err_.map_err(map_void_f).unwrap_err(), "a");
}

TEST_F(ResultTest, Pointer) {
  auto value = std::make_unique<int>(1);
  auto result = Ok<int *, int>(value.get());
  ASSERT_EQ(*result.unwrap(), 1);
}