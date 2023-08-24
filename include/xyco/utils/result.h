#ifndef XYCO_UTILS_RESULT_H_
#define XYCO_UTILS_RESULT_H_

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRY(result_expr)               \
  ({                                   \
    auto owned_result = (result_expr); \
    if (!owned_result) {               \
      return std::move(owned_result);  \
    }                                  \
    *std::move(owned_result);          \
  })

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASYNC_TRY(result_expr)           \
  ({                                     \
    auto owned_result = (result_expr);   \
    if (!owned_result) {                 \
      co_return std::move(owned_result); \
    }                                    \
    *std::move(owned_result);            \
  })

#endif  // XYCO_UTILS_RESULT_H_
