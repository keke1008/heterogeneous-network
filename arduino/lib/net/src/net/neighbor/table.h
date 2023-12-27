#pragma once

#include "./constants.h"
#include <memory/pair_shared.h>
#include <net/link.h>
#include <net/node.h>
#include <tl/vec.h>

namespace net::neighbor {
    class NeighborNode {
        node::NodeId id_;
        node::Cost link_cost_;
        tl::Vec<link::Address, MAX_MEDIA_PER_NODE> addresses_;

      public:
        explicit NeighborNode(const node::NodeId &id, node::Cost link_cost)
            : id_{id},
              link_cost_{link_cost} {}

        inline const node::NodeId &id() const {
            return id_;
        }

        inline node::Cost link_cost() const {
            return link_cost_;
        }

        inline void set_link_cost(node::Cost cost) {
            link_cost_ = cost;
        }

        inline etl::span<const link::Address> addresses() const {
            return addresses_.as_span();
        }

        inline bool has_address(const link::Address &address) const {
            return etl::any_of(
                addresses_.begin(), addresses_.end(),
                [&](const link::Address &addr) { return addr == address; }
            );
        }

        inline void add_address_if_not_exists(const link::Address &address) {
            if (!(addresses_.full() || has_address(address))) {
                addresses_.push_back(address);
            }
        }

        inline bool has_addresses_type(link::AddressType type) const {
            return etl::any_of(
                addresses_.begin(), addresses_.end(),
                [&](const link::Address &addr) { return addr.type() == type; }
            );
        }
    };

    enum class AddLinkResult : uint8_t {
        NewNodeConnected,
        CostUpdated,
        AlreadyConnected,
        Full,
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

    class NeighborList {
        tl::Vec<NeighborNode, MAX_NEIGHBOR_NODE_COUNT> neighbors_;
        NeighborListCursotStorage cursors_;

        inline etl::optional<uint8_t> find_neighbor_node(const node::NodeId &node_id) const {
            for (uint8_t i = 0; i < neighbors_.size(); i++) {
                if (neighbors_[i].id() == node_id) {
                    return i;
                }
            }
            return etl::nullopt;
        }

      public:
        AddLinkResult add_neighbor_link(
            const node::NodeId &node_id,
            const link::Address &address,
            node::Cost cost
        ) {
            if (neighbors_.full()) {
                return AddLinkResult::Full;
            }

            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors_[opt_index.value()];
                node.add_address_if_not_exists(address);

                if (node.link_cost() == cost) {
                    return AddLinkResult::AlreadyConnected;
                } else {
                    node.set_link_cost(cost);
                    return AddLinkResult::CostUpdated;
                }
            }

            neighbors_.emplace_back(node_id, cost);
            neighbors_.back().add_address_if_not_exists(address);
            return AddLinkResult::NewNodeConnected;
        }

        RemoveNodeResult remove_neighbor_node(const node::NodeId &node_id) {
            auto opt_index = find_neighbor_node(node_id);
            if (!opt_index.has_value()) {
                return RemoveNodeResult::NotFound;
            }

            auto index = opt_index.value();
            neighbors_.remove(index);
            cursors_.sync_index_on_element_removed(index);

            return RemoveNodeResult::Disconnected;
        }

        inline bool has_neighbor_node(const node::NodeId &node_id) const {
            return find_neighbor_node(node_id).has_value();
        }

        inline etl::optional<node::Cost> get_link_cost(const node::NodeId &neighbor_id) const {
            auto index = find_neighbor_node(neighbor_id);
            return index ? etl::optional(neighbors_[*index].link_cost()) : etl::nullopt;
        }

        etl::optional<etl::span<const link::Address>> get_addresses_of(const node::NodeId &node_id
        ) const {
            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors_[opt_index.value()];
                return node.addresses();
            } else {
                return etl::nullopt;
            }
        }

        inline nb::Poll<NeighborListCursor> poll_cursor() {
            return cursors_.poll_create_cursor(neighbors_.size());
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        get_neighbor_node(const NeighborListCursor &cursor) const {
            return neighbors_.at(cursor.get());
        }
    };
} // namespace net::neighbor
