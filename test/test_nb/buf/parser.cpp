#include <doctest.h>
#include <util/doctest_ext.h>

#include <nb/buf.h>

struct AsyncBuffer {
    etl::array<uint8_t, 10> buffer{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t length{0};

    etl::span<const uint8_t> span() const {
        return etl::span(buffer).first(length);
    }
};

static_assert(nb::buf::IAsyncBuffer<AsyncBuffer>);

struct Uint8Parser {
    etl::optional<uint8_t> result_;

    template <nb::buf::IAsyncBuffer Buffer>
    nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
        if (result_.has_value()) {
            return nb::ready();
        }
        result_ = POLL_UNWRAP_OR_RETURN(splitter.split_1byte());
        return nb::ready();
    }

    uint8_t result() {
        return result_.value();
    }
};

static_assert(nb::buf::IAsyncParser<Uint8Parser, AsyncBuffer>);

struct Sum3Uint8Parser {
    Uint8Parser a;
    Uint8Parser b;
    Uint8Parser c;

    template <nb::buf::IAsyncBuffer Buffer>
    nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
        POLL_UNWRAP_OR_RETURN(a.parse(splitter));
        POLL_UNWRAP_OR_RETURN(b.parse(splitter));
        return c.parse(splitter);
    }

    uint8_t result() {
        return a.result() + b.result() + c.result();
    }
};

static_assert(nb::buf::IAsyncParser<Sum3Uint8Parser, AsyncBuffer>);

TEST_CASE("AsyncParser") {
    AsyncBuffer buf;
    nb::buf::AsyncBufferSplitter splitter{buf};
    Sum3Uint8Parser parser;

    CHECK(splitter.parse(parser).is_pending());

    buf.length = 1;
    CHECK(splitter.parse(parser).is_pending());
    CHECK(parser.a.result() == 1);

    buf.length = 2;
    CHECK(splitter.parse(parser).is_pending());
    CHECK(parser.b.result() == 2);

    buf.length = 3;
    CHECK(splitter.parse(parser).is_ready());
    CHECK(parser.c.result() == 3);
    CHECK(parser.result() == 6);
}
