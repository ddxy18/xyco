#include "registry.h"

template <typename FormatContext>
auto fmt::formatter<xyco::runtime::Event>::format(
    const xyco::runtime::Event &event, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "Event{{extra_={}}}", event.extra_->print());
}

template auto fmt::formatter<xyco::runtime::Event>::format(
    const xyco::runtime::Event &event,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());