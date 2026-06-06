/*
 *   EntityPool — generic entity array management with index-based iteration
 *
 *   Encapsulates the common pattern used by enemies, bullets, lasers, items,
 *   fragments, and effects:
 *     1. Fixed-size array of entities
 *     2. Index array for compact iteration
 *     3. Active count
 *     4. InitIndices / Alloc / Compact lifecycle
 */

#pragma once

#include "GIAN07/game/entity.h" // for Indsort
#include <array>
#include <cstdint>
#include <cstring>

template <typename T, size_t N> struct EntityPool {
  std::array<T, N> entities;
  std::array<uint16_t, N> indices;
  uint16_t count = 0;

  // Initialize indices and reset count
  void InitIndices() {
    for (uint16_t i = 0; i < N; i++) {
      indices[i] = i;
    }
    count = 0;
  }

  // Get next free entity slot. Returns nullptr if pool is full.
  T *Alloc() {
    if (count + 1 >= N) {
      return nullptr;
    }
    return &entities[indices[count++]];
  }

  // Compact the pool by removing entities matching the predicate
  template <typename Pred> void Compact(Pred pred) {
    Indsort(indices, count, entities, pred);
  }

  // Number of active entities
  uint16_t Size() const { return count; }

  // Access active entity by index (0-based active index, not slot index)
  T &operator[](uint16_t i) { return entities[indices[i]]; }
  const T &operator[](uint16_t i) const { return entities[indices[i]]; }

  // Range-based for support over active entities
  struct Iterator {
    EntityPool *pool;
    uint16_t pos;
    T &operator*() { return (*pool)[pos]; }
    T *operator->() { return &(*pool)[pos]; }
    Iterator &operator++() {
      pos++;
      return *this;
    }
    bool operator!=(const Iterator &o) const { return pos != o.pos; }
  };

  struct ConstIterator {
    const EntityPool *pool;
    uint16_t pos;
    const T &operator*() { return (*pool)[pos]; }
    const T *operator->() { return &(*pool)[pos]; }
    ConstIterator &operator++() {
      pos++;
      return *this;
    }
    bool operator!=(const ConstIterator &o) const { return pos != o.pos; }
  };

  Iterator begin() { return {this, 0}; }
  Iterator end() { return {this, count}; }
  ConstIterator begin() const { return {this, 0}; }
  ConstIterator end() const { return {this, count}; }
};
