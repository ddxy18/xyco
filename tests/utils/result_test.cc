#include "utils/result.h"

#include <gtest/gtest.h>


TEST(ResultTest, IsOk) {
  auto result = Ok<int, int>(1);
  ASSERT_TRUE(result.is_ok());
}

TEST(ResultTest, Unwrap) {
  auto result = Ok<int, int>(1);
  ASSERT_EQ(result.unwrap(), 1);
}

TEST(ResultTest, UnwrapErr) {
  try {
    Err<int, int>(1).unwrap();
  } catch (std::runtime_error e) {
    ASSERT_EQ(std::string(e.what()), "panic");
  }
}

TEST(ResultTest, UnwrapOr) {
  auto result = Err<std::string, int>(0).unwrap_or(std::string("ab"));
  ASSERT_EQ(result.length(), 2);
}

TEST(ResultTest, Map) {
  auto result = Ok<int, int>(1);
  ASSERT_EQ(
      result.map(std::function<const char *(int)>([](auto n) { return "a"; }))
          .unwrap(),
      "a");
}

TEST(ResultTest, MapErr) {
  auto result = Err<int, int>(1);
  ASSERT_EQ(
      result
          .map_err(std::function<const char *(int)>([](auto n) { return "a"; }))
          .unwrap_err(),
      "a");
}

TEST(ResultTest, Pointer) {
  auto ok = std::make_unique<int>(1);
  auto result = Ok<int *, int>(ok.get());
  ASSERT_EQ(*result.unwrap(), 1);
}

TEST(ResultTest, VoidOk) {
  auto result = Ok<Void, int>(Void());
  ASSERT_TRUE(result.is_ok());
}

TEST(ResultTest, VoidErr) {
  auto result = Err<int, Void>(Void());
  ASSERT_TRUE(result.is_err());
}
