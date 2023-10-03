#pragma once

#include <nb/poll.h>
#include <net/frame/service.h>
#include <util/concepts.h>
#include <util/time.h>

namespace net::media {
    template <typename Media, typename FrameService>
    concept MediaInterface = frame::IFrameService<FrameService> &&
        requires(Media media, util::Time &time, FrameService &frame_service) {
            { media.wait_for_media_detection(time) } -> util::same_as<nb::Poll<void>>;
            { media.get_address() } -> util::same_as<typename FrameService::Address>;
            { media.execute(frame_service) } -> util::same_as<void>;
        };

    class MediaHandle {
        friend class MediaHandleIterator;

        uint8_t index_;

      public:
        MediaHandle() = delete;
        MediaHandle(const MediaHandle &) = default;
        MediaHandle(MediaHandle &&) = default;
        MediaHandle &operator=(const MediaHandle &) = default;
        MediaHandle &operator=(MediaHandle &&) = default;

        explicit MediaHandle(uint8_t value) : index_{value} {}

        inline bool operator==(const MediaHandle &other) const {
            return index_ == other.index_;
        }

        inline bool operator!=(const MediaHandle &other) const {
            return !(*this == other);
        }
    };

    class MediaHandleIterator {
        MediaHandle handle_;

      public:
        explicit MediaHandleIterator(MediaHandle handle) : handle_{handle} {}

        inline MediaHandleIterator &operator++() {
            handle_ = MediaHandle{static_cast<uint8_t>(handle_.index_ + 1)};
            return *this;
        }

        inline MediaHandleIterator operator++(int) {
            MediaHandleIterator tmp{*this};
            ++(*this);
            return tmp;
        }

        inline bool operator==(const MediaHandleIterator &other) const {
            return handle_ == other.handle_;
        }

        inline bool operator!=(const MediaHandleIterator &other) const {
            return !(*this == other);
        }

        inline MediaHandle operator*() const {
            return handle_;
        }
    };

    template <typename Service>
    concept IMediaService = requires(Service &service) {
        { service.begin() } -> util::same_as<MediaHandleIterator>;
        { service.end() } -> util::same_as<MediaHandleIterator>;
        { service.wait_for_media_detection() } -> util::same_as<nb::Poll<void>>;
    };

    template <
        uint8_t MEDIA_COUNT,
        frame::IFrameService FrameService,
        MediaInterface<FrameService> Media>
    class MediaService {
        const etl::span<Media, MEDIA_COUNT> media_;

      public:
        using Address = typename FrameService::Address;

        MediaService() = delete;
        MediaService(const MediaService &) = delete;
        MediaService(MediaService &&) = delete;
        MediaService &operator=(const MediaService &) = delete;
        MediaService &operator=(MediaService &&) = delete;

        MediaService(etl::span<Media, MEDIA_COUNT> media) : media_{media} {}

        inline MediaHandleIterator begin() const {
            return MediaHandleIterator{MediaHandle{0}};
        }

        inline MediaHandleIterator end() const {
            return MediaHandleIterator{MediaHandle{MEDIA_COUNT}};
        }

        inline nb::Poll<void> wait_for_media_detection() {
            for (auto &media : media_) {
                POLL_UNWRAP_OR_RETURN(media.wait_for_media_detection());
            }
            return nb::ready();
        }

        inline etl::optional<MediaHandle> address_to_handle(Address &address) const {
            for (auto &media : media_) {
                if (media.get_address() == address) {
                    return MediaHandle{static_cast<uint8_t>(&media - media_.data())};
                }
            }
            return etl::nullopt;
        }

        inline void execute(FrameService &frame_service) {
            for (auto &media : media_) {
                media.execute(frame_service);
            }
        }
    };
} // namespace net::media
