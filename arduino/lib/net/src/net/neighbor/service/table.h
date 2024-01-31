#pragma once

#include "../constants.h"
#include <memory/pair_shared.h>
#include <net/link.h>
#include <net/node.h>
#include <net/notification.h>
#include <tl/vec.h>

namespace net::neighbor {
    struct NeighborNodeAddress {
        link::Address address;
        link::MediaPortMask gateway_port_mask;
    };

    struct NeighborNodeAddresses {
        friend class NeighborNode;

        tl::Vec<NeighborNodeAddress, MAX_MEDIA_PER_NODE> addresses;
        link::AddressTypeSet address_types;

        inline bool has_address(const link::Address &address) const {
            if (!address_types.test(address.type())) {
                return false;
            }

            return etl::any_of(
                addresses.begin(), addresses.end(),
                [&](const NeighborNodeAddress &addr) { return addr.address == address; }
            );
        }

      private:
        void update(const link::Address &address, link::MediaPortMask gateway_port_mask) {
            for (auto &addr : addresses) {
                if (addr.address == address) {
                    addr.gateway_port_mask |= gateway_port_mask;
                    return;
                }
            }

            addresses.emplace_back(address, gateway_port_mask);
            address_types.set(address.type());
        }

        inline bool overlap_addresses_type(const link::AddressTypeSet &types) const {
            return (address_types & types).any();
        }

        inline etl::span<const NeighborNodeAddress> as_span() const {
            return addresses.as_span();
        }
    };

    struct NeighborNodeTimer {
        nb::Delay expiration_timeout;
        nb::Delay send_hello_interval;

        explicit NeighborNodeTimer(util::Time &time)
            : expiration_timeout{time, NEIGHBOR_EXPIRATION_TIMEOUT},
              send_hello_interval{time, SEND_HELLO_INTERVAL} {}

        inline bool is_expired(util::Time &time) const {
            return expiration_timeout.poll(time).is_ready();
        }

        inline bool should_send_hello(util::Time &time) const {
            return send_hello_interval.poll(time).is_ready();
        }

        inline void reset_expiration(util::Time &time) {
            expiration_timeout = nb::Delay{time, NEIGHBOR_EXPIRATION_TIMEOUT};
        }

        inline void reset_send_hello_interval(util::Time &time) {
            send_hello_interval = nb::Delay{time, SEND_HELLO_INTERVAL};
        }
    };

    class NeighborNode {
        friend class NeighborList;

        node::NodeId id_;
        node::Cost link_cost_;
        NeighborNodeAddresses addresses_;
        NeighborNodeTimer timer_;

      public:
        explicit NeighborNode(
            const node::NodeId &id,
            node::Cost link_cost,
            link::Address address,
            link::MediaPortMask gateway_port_mask,
            util::Time &time
        )
            : id_{id},
              link_cost_{link_cost},
              addresses_{},
              timer_{time} {
            addresses_.update(address, gateway_port_mask);
        }

        inline const node::NodeId &id() const {
            return id_;
        }

        inline node::Cost link_cost() const {
            return link_cost_;
        }

      private:
        inline void set_link_cost(node::Cost cost) {
            link_cost_ = cost;
        }

      public:
        inline etl::span<const NeighborNodeAddress> addresses() const {
            return addresses_.as_span();
        }

        inline bool has_address(const link::Address &address) const {
            return addresses_.has_address(address);
        }

        inline void
        update_address(const link::Address &address, link::MediaPortMask gateway_port_mask) {
            addresses_.update(address, gateway_port_mask);
        }

        inline bool overlap_addresses_type(link::AddressTypeSet types) const {
            return addresses_.overlap_addresses_type(types);
        }

        inline bool is_expired(util::Time &time) const {
            return timer_.is_expired(time);
        }

        inline bool should_send_hello(util::Time &time) const {
            return timer_.should_send_hello(time);
        }

      private:
        inline void delay_expiration(util::Time &time) {
            timer_.reset_expiration(time);
        }

        inline void delay_hello_interval(util::Time &time) {
            timer_.reset_send_hello_interval(time);
        }
    };

    enum class AddLinkResult : uint8_t {
        NewNodeConnected,
        CostUpdated,
        NoChange,
    };

    enum class RemoveNodeResult : uint8_t {
        Disconnected,
        NotFound,
    };

    class NeighborListCursor {
        friend class NeighborListCursotStorage;

        memory::Owned<uint8_t> index_;

        explicit NeighborListCursor(memory::Owned<uint8_t> &&index) : index_{etl::move(index)} {}

      public:
        inline void advance() {
            ++index_.get();
        }

        inline uint8_t get() const {
            return index_.get();
        }

        inline void reset() {
            index_.get() = 0;
        }
    };

    class NeighborListCursorRef {
        friend class NeighborListCursotStorage;

        memory::Reference<uint8_t> index_;

        explicit NeighborListCursorRef(memory::Reference<uint8_t> &&index)
            : index_{etl::move(index)} {}

        inline void sync_index_on_element_removed(uint8_t removed_index) {
            auto opt_index = index_.get();
            if (!opt_index.has_value()) {
                return;
            }

            auto &index = opt_index->get();
            if (index > removed_index) {
                --index;
            }
        }

        inline bool is_out_of_range(uint8_t size) const {
            auto opt_index = index_.get();
            return opt_index.has_value() ? opt_index->get() >= size : true;
        }
    };

    class NeighborListCursotStorage {
        tl::Vec<NeighborListCursorRef, MAX_NEIGHBOR_LIST_CURSOR_COUNT> cursors_;

        inline void remove_out_of_range_cursors(uint8_t size) {
            for (uint8_t i = 0; i < cursors_.size(); ++i) {
                if (cursors_[i].is_out_of_range(size)) {
                    cursors_.remove(i);
                }
            }
        }

      public:
        inline nb::Poll<NeighborListCursor> poll_create_cursor(uint8_t neighbors_size) {
            if (cursors_.full()) {
                remove_out_of_range_cursors(neighbors_size);
                if (cursors_.full()) {
                    return nb::pending;
                }
            }

            auto [owned, ref] = memory::make_shared<uint8_t>(0);
            cursors_.push_back(NeighborListCursorRef{etl::move(ref)});
            return NeighborListCursor{etl::move(owned)};
        }

        inline void sync_index_on_element_removed(uint8_t removed_index) {
            for (auto &cursor : cursors_) {
                cursor.sync_index_on_element_removed(removed_index);
            }
        }
    };

    class PointableNeighbors {
        tl::Vec<NeighborNode, MAX_NEIGHBOR_NODE_COUNT> neighbors_{};
        NeighborListCursotStorage cursors_{};

      public:
        bool full() const {
            return neighbors_.full();
        }

        inline etl::optional<uint8_t> find_index(const node::NodeId &node_id) const {
            for (uint8_t i = 0; i < neighbors_.size(); i++) {
                if (neighbors_[i].id() == node_id) {
                    return i;
                }
            }
            return etl::nullopt;
        }

        inline etl::optional<etl::reference_wrapper<NeighborNode>> find(const node::NodeId &node_id
        ) {
            auto opt_index = find_index(node_id);
            return opt_index ? etl::ref(neighbors_[*opt_index])
                             : etl::optional<etl::reference_wrapper<NeighborNode>>{};
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        find(const node::NodeId &node_id) const {
            auto opt_index = find_index(node_id);
            return opt_index ? etl::optional(etl::cref(neighbors_[*opt_index])) : etl::nullopt;
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        find_by_address(const link::Address &address) const {
            for (uint8_t i = 0; i < neighbors_.size(); i++) {
                if (neighbors_[i].has_address(address)) {
                    return etl::cref(neighbors_[i]);
                }
            }
            return etl::nullopt;
        }

        inline NeighborNode &emplace_back_neighbor(
            const node::NodeId &node_id,
            node::Cost link_cost,
            link::Address address,
            link::MediaPortMask gateway_port_mask,
            util::Time &time
        ) {
            FASSERT(!full());
            neighbors_.emplace_back(node_id, link_cost, address, gateway_port_mask, time);
            return neighbors_.back();
        }

        inline void remove_neighbor(uint8_t index) {
            neighbors_.remove(index);
            cursors_.sync_index_on_element_removed(index);
        }

        inline nb::Poll<NeighborListCursor> poll_cursor() {
            return cursors_.poll_create_cursor(neighbors_.size());
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        get_by_cursor(const NeighborListCursor &cursor) const {
            auto index = cursor.get();
            return index < neighbors_.size() ? etl::optional(etl::cref(neighbors_[index]))
                                             : etl::nullopt;
        }

        inline NeighborNode &get_by_index(uint8_t index) {
            FASSERT(index < neighbors_.size());
            return neighbors_[index];
        }

        inline etl::span<NeighborNode> as_span() {
            return neighbors_.as_span();
        }

        inline etl::span<const NeighborNode> as_span() const {
            return neighbors_.as_span();
        }

        inline uint8_t size() const {
            return neighbors_.size();
        }
    };

    enum AddNeighborResult {
        NoChange,
        Updated,
        Full,
    };

    class NeighborList {
        PointableNeighbors neighbors_{};
        nb::Debounce check_expired_debounce_;

      public:
        explicit NeighborList(util::Time &time)
            : check_expired_debounce_{time, CHECK_NEIGHBOR_EXPIRATION_INTERVAL} {}

        AddNeighborResult add_neighbor(
            const node::NodeId &node_id,
            node::Cost link_cost,
            const link::Address &address,
            link::MediaPortMask gateway_port_mask,
            util::Time &time
        ) {
            if (neighbors_.full()) {
                return AddNeighborResult::Full;
            }

            auto opt_neighbor = neighbors_.find(node_id);
            if (!opt_neighbor.has_value()) {
                neighbors_.emplace_back_neighbor(
                    node_id, link_cost, address, gateway_port_mask, time
                );
                LOG_INFO(FLASH_STRING("new neigh: "), node_id);
                return AddNeighborResult::Updated;
            }

            auto &node = opt_neighbor.value().get();
            node.update_address(address, gateway_port_mask);
            if (node.link_cost() == link_cost) {
                return AddNeighborResult::NoChange;
            }

            node.set_link_cost(link_cost);
            return AddNeighborResult::Updated;
        }

        inline bool has_neighbor_node(const node::NodeId &node_id) const {
            return neighbors_.find_index(node_id).has_value();
        }

        inline etl::optional<node::Cost> get_link_cost(const node::NodeId &neighbor_id) const {
            auto opt_neighbor = neighbors_.find(neighbor_id);
            return opt_neighbor ? etl::optional(opt_neighbor->get().link_cost()) : etl::nullopt;
        }

        etl::optional<etl::span<const NeighborNodeAddress>>
        get_addresses_of(const node::NodeId &node_id) const {
            auto opt_neighbor = neighbors_.find(node_id);
            return opt_neighbor ? etl::optional(opt_neighbor->get().addresses()) : etl::nullopt;
        }

        inline nb::Poll<NeighborListCursor> poll_cursor() {
            return neighbors_.poll_cursor();
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        get_neighbor_node(const NeighborListCursor &cursor) const {
            return neighbors_.get_by_cursor(cursor);
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        get_neighbor_node(const node::NodeId &neighbor_id) const {
            return neighbors_.find(neighbor_id);
        }

        inline uint8_t get_neighbor_count() const {
            return neighbors_.size();
        }

        template <typename F>
        inline void for_each_neighbor_node(F &&f) const {
            for (auto &neighbor : neighbors_.as_span()) {
                f(neighbor);
            }
        }

        inline etl::optional<etl::reference_wrapper<NeighborNode>>
        get_neighbor_node(const node::NodeId &neighbor_id) {
            return neighbors_.find(neighbor_id);
        }

        inline void delay_hello_interval(link::AddressTypeSet types, util::Time &time) {
            for (auto &neighbor : neighbors_.as_span()) {
                if (neighbor.overlap_addresses_type(types)) {
                    neighbor.delay_hello_interval(time);
                }
            }
        }

        inline void delay_hello_interval(link::AddressType type, util::Time &time) {
            delay_hello_interval(link::AddressTypeSet{type}, time);
        }

        inline void delay_hello_interval(const node::NodeId &node_id, util::Time &time) {
            auto opt_neighbor = neighbors_.find(node_id);
            if (opt_neighbor) {
                opt_neighbor->get().delay_hello_interval(time);
            }
        }

        inline void delay_expiration(const node::NodeId &node_id, util::Time &time) {
            auto opt_neighbor = neighbors_.find(node_id);
            if (opt_neighbor) {
                opt_neighbor->get().delay_expiration(time);
            }
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        resolve_neighbor_node_from_address(const link::Address &address) const {
            return neighbors_.find_by_address(address);
        }

        void execute(notification::NotificationService &nts, util::Time &time) {
            if (check_expired_debounce_.poll(time).is_pending()) {
                return;
            }

            uint8_t index = 0;
            while (index < neighbors_.size()) {
                auto &neighbor = neighbors_.get_by_index(index);
                if (neighbor.is_expired(time)) {
                    nts.notify(notification::NeighborRemoved{neighbor.id()});
                    neighbors_.remove_neighbor(index);
                } else {
                    ++index;
                }
            }
        }
    };
} // namespace net::neighbor
