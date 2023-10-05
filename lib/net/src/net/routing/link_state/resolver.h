#pragma once

#include "../node.h"
#include <etl/limits.h>
#include <etl/priority_queue.h>
#include <net/link.h>

namespace net::routing::resolver {
    template <uint8_t SIZE>
    class AdjacencyCost {
        etl::array<etl::array<Cost, SIZE>, SIZE> matrix_;

      public:
        AdjacencyCost() {
            etl::for_each(matrix_.begin(), matrix_.end(), [](etl::array<Cost, SIZE> &row) {
                row.fill(Cost::infinite());
            });
        }

        inline constexpr Cost get(uint8_t vertex1, uint8_t vertex2) const {
            return matrix_[vertex1][vertex2];
        }

        inline constexpr Cost get_vertex(uint8_t vertex) const {
            return matrix_[vertex][vertex];
        }

        inline constexpr bool set(uint8_t vertex1, uint8_t vertex2, Cost cost) {
            Cost prev = matrix_[vertex1][vertex2];
            matrix_[vertex1][vertex2] = cost;
            matrix_[vertex2][vertex1] = cost;
            return prev != cost;
        }

        inline constexpr bool set_vertex(uint8_t vertex, Cost cost) {
            Cost prev = matrix_[vertex][vertex];
            matrix_[vertex][vertex] = cost;
            return prev != cost;
        }

        inline constexpr void remove_vertex(uint8_t vertex) {
            for (uint8_t i = 0; i < SIZE; i++) {
                matrix_[vertex][i] = Cost::infinite();
                matrix_[i][vertex] = Cost::infinite();
            }
        }
    };

    template <uint8_t SIZE>
    etl::optional<uint8_t> resolve_gateway_vertex(
        const AdjacencyCost<SIZE> &cost,
        const etl::span<const bool, SIZE> valid_vertex,
        const uint8_t initial_vertex,
        const uint8_t destination_vertex
    ) {
        struct Vertex {
            uint16_t cost;
            uint8_t index;
            uint8_t gateway;

          public:
            inline constexpr bool operator<(const Vertex &other) const {
                return cost < other.cost;
            }
        };

        etl::priority_queue<Vertex, SIZE, etl::vector<Vertex, SIZE>, etl::greater<Vertex>> queue;
        queue.push({
            .cost = 0,
            .index = initial_vertex,
            .gateway = initial_vertex,
        });

        etl::array<uint16_t, SIZE> min_cost;
        etl::array<bool, SIZE> unvisited;
        unvisited.assign(valid_vertex.begin(), valid_vertex.end(), false);

        min_cost.fill(etl::numeric_limits<uint16_t>::max());
        min_cost[initial_vertex] = 0;

        while (!queue.empty()) {
            Vertex v = queue.top();
            queue.pop();

            if (!unvisited[v.index] || v.cost > min_cost[v.index]) {
                continue;
            }
            unvisited[v.index] = false;

            if (v.index == destination_vertex) {
                return v.gateway;
            }

            for (uint8_t i = 0; i < SIZE; i++) {
                if (!unvisited[i] || cost.get(v.index, i).is_infinite()) {
                    continue;
                }

                uint16_t new_cost =
                    v.cost + cost.get_vertex(v.index).value() + cost.get(v.index, i).value();
                if (new_cost < min_cost[i]) {
                    min_cost[i] = new_cost;
                    queue.push({
                        .cost = new_cost,
                        .index = i,
                        .gateway = v.gateway == initial_vertex ? i : v.gateway,
                    });
                }
            }
        }

        return etl::nullopt;
    }
} // namespace net::routing::resolver
