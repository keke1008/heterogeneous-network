#pragma once

#include <nb/poll.h>
#include <tl/vec.h>
#include <util/time.h>

namespace nb {
    class Delay {
        util::Instant start_;
        util::Duration duration_;

      public:
        explicit inline Delay(util::Time &time, util::Duration duration)
            : start_{time.now()},
              duration_{duration} {}

        explicit inline Delay(util::Instant start, util::Duration duration)
            : start_{start},
              duration_{duration} {}

        inline nb::Poll<void> poll(util::Instant now) const {
            if (now - start_ >= duration_) {
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        inline nb::Poll<void> poll(util::Time &time) const {
            return poll(time.now());
        }

        inline void set_duration(util::Duration duration) {
            duration_ = duration;
        }
    };

    class Debounce {
        util::Instant last_;
        util::Duration duration_;

      public:
        explicit inline Debounce(util::Time &time, util::Duration duration)
            : last_{time.now()},
              duration_{duration} {}

        inline nb::Poll<void> poll(util::Time &time) {
            util::Instant now = time.now();
            if (now - last_ >= duration_) {
                last_ = now;
                return nb::ready();
            } else {
                return nb::pending;
            }
        }
    };

    template <typename T>
    struct DelayPoolEntry {
        T value;
        nb::Delay delay;
    };

    template <typename T, uint8_t MAX_CAPACITY>
    class DelayPoolSeeker {
        util::Instant now_;
        tl::Vec<DelayPoolEntry<T>, MAX_CAPACITY> &entries_;
        uint8_t index_ = 0;

        void seek_next() {
            for (; index_ < entries_.size(); ++index_) {
                DelayPoolEntry<T> &entry = entries_[index_];
                if (entry.delay.poll(now_).is_ready()) {
                    return;
                }
            }
        }

      public:
        explicit DelayPoolSeeker(
            util::Instant now,
            tl::Vec<DelayPoolEntry<T>, MAX_CAPACITY> &entries
        )
            : now_{now},
              entries_{entries} {
            seek_next();
        }

        inline etl::optional<etl::reference_wrapper<T>> current() {
            return index_ == entries_.size() ? etl::nullopt
                                             : etl::optional(etl::ref(entries_[index_].value));
        }

        inline void remove_and_next() {
            entries_.swap_remove(index_);
            seek_next();
        }
    };

    template <typename T, uint8_t MAX_CAPACITY>
    class DelayPool {
        tl::Vec<DelayPoolEntry<T>, MAX_CAPACITY> entries_;

      public:
        inline nb::Poll<void> poll_pushable() const {
            return entries_.full() ? nb::pending : nb::ready();
        }

        inline void push(T &&value, util::Duration delay, util::Time &time) {
            FASSERT(poll_pushable().is_ready());
            entries_.emplace_back(etl::move(value), nb::Delay{time, delay});
        }

        inline DelayPoolSeeker<T, MAX_CAPACITY> seek(util::Time &time) {
            return DelayPoolSeeker<T, MAX_CAPACITY>{time.now(), entries_};
        }

        inline nb::Poll<T> poll_pop_expired(util::Time &time) {
            util::Instant now = time.now();

            for (uint8_t i = 0; i < entries_.size(); ++i) {
                DelayPoolEntry<T> &entry = entries_[i];
                if (entry.delay.poll(now).is_ready()) {
                    return entries_.swap_remove(i).value;
                }
            }
            return nb::pending;
        }
    };

} // namespace nb
