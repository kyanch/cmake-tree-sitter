#include <stdbool.h>
#include "runtime/subtree.h"
#include "runtime/tree.h"
#include "runtime/language.h"

// NodeChildIterator

typedef struct {
  const Subtree *parent;
  const TSTree *tree;
  Length position;
  uint32_t child_index;
  uint32_t structural_child_index;
  const TSSymbol *alias_sequence;
} NodeChildIterator;

// TSNode - Private

static inline TSNode ts_node__null() {
  return (TSNode) {
    .subtree = NULL,
    .tree = NULL,
    .position = {0, 0},
    .byte = 0,
  };
}

static inline const Subtree *ts_node__tree(TSNode self) {
  return self.subtree;
}

static inline NodeChildIterator ts_node_child_iterator_begin(const TSNode *node) {
  const Subtree *tree = ts_node__tree(*node);
  const TSSymbol *alias_sequence = ts_language_alias_sequence(
    node->tree->language,
    tree->alias_sequence_id
  );
  return (NodeChildIterator) {
    .parent = tree,
    .tree = node->tree,
    .position = {node->byte, node->position},
    .child_index = 0,
    .structural_child_index = 0,
    .alias_sequence = alias_sequence,
  };
}

static inline bool ts_node_child_iterator_next(NodeChildIterator *self, TSNode *result) {
  if (self->child_index == self->parent->children.size) return false;
  const Subtree *child = self->parent->children.contents[self->child_index];
  TSSymbol alias_symbol = 0;
  if (!child->extra) {
    if (self->alias_sequence) {
      alias_symbol = self->alias_sequence[self->structural_child_index];
    }
    self->structural_child_index++;
  }
  *result = (TSNode) {
    .subtree = child,
    .tree = self->tree,
    .position = self->position.extent,
    .byte = self->position.bytes,
    .alias_symbol = alias_symbol,
  };
  self->position = length_add(self->position, ts_subtree_total_size(child));
  self->child_index++;
  return true;
}

static inline bool ts_node__is_relevant(TSNode self, bool include_anonymous) {
  const Subtree *tree = ts_node__tree(self);
  if (include_anonymous) {
    return tree->visible || self.alias_symbol;
  } else {
    return (
      (tree->visible && tree->named) ||
      (
        self.alias_symbol &&
        ts_language_symbol_metadata(
          self.tree->language,
          self.alias_symbol
        ).named
      )
    );
  }
}

static inline uint32_t ts_node__relevant_child_count(TSNode self, bool include_anonymous) {
  const Subtree *tree = ts_node__tree(self);
  if (tree->children.size > 0) {
    if (include_anonymous) {
      return tree->visible_child_count;
    } else {
      return tree->named_child_count;
    }
  } else {
    return 0;
  }
}

static inline TSNode ts_node__child(TSNode self, uint32_t child_index, bool include_anonymous) {
  TSNode result = self;
  bool did_descend = true;

  while (did_descend) {
    did_descend = false;

    TSNode child;
    uint32_t index = 0;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&result);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      if (ts_node__is_relevant(child, include_anonymous)) {
        if (index == child_index) return child;
        index++;
      } else {
        uint32_t grandchild_index = child_index - index;
        uint32_t grandchild_count = ts_node__relevant_child_count(child, include_anonymous);
        if (grandchild_index < grandchild_count) {
          did_descend = true;
          result = child;
          child_index = grandchild_index;
          break;
        }
        index += grandchild_count;
      }
    }
  }

  return ts_node__null();
}

static inline TSNode ts_node__prev_sibling(TSNode self, bool include_anonymous) {
  uint32_t target_end_byte = ts_node_end_byte(self);

  TSNode node = ts_node_parent(self);
  TSNode earlier_node = ts_node__null();
  bool earlier_node_is_relevant = false;

  while (node.subtree) {
    TSNode earlier_child = ts_node__null();
    bool earlier_child_is_relevant = false;
    bool found_child_containing_target = false;

    TSNode child;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&node);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      if (iterator.position.bytes >= target_end_byte) {
        found_child_containing_target = child.subtree != self.subtree;
        break;
      }

      if (ts_node__is_relevant(child, include_anonymous)) {
        earlier_child = child;
        earlier_child_is_relevant = true;
      } else if (ts_node__relevant_child_count(child, include_anonymous) > 0) {
        earlier_child = child;
        earlier_child_is_relevant = false;
      }
    }

    if (found_child_containing_target) {
      if (earlier_child.subtree) {
        earlier_node = earlier_child;
        earlier_node_is_relevant = earlier_child_is_relevant;
      }
      node = child;
    } else if (earlier_child_is_relevant) {
      return earlier_child;
    } else if (earlier_child.subtree) {
      node = earlier_child;
    } else if (earlier_node_is_relevant) {
      return earlier_node;
    } else {
      node = earlier_node;
    }
  }

  return ts_node__null();
}

static inline TSNode ts_node__next_sibling(TSNode self, bool include_anonymous) {
  uint32_t target_end_byte = ts_node_end_byte(self);

  TSNode node = ts_node_parent(self);
  TSNode later_node = ts_node__null();
  bool later_node_is_relevant = false;

  while (node.subtree) {
    TSNode later_child = ts_node__null();
    bool later_child_is_relevant = false;
    TSNode child_containing_target = ts_node__null();

    TSNode child;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&node);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      if (iterator.position.bytes < target_end_byte) continue;
      if (child.byte <= self.byte) {
        if (child.subtree != self.subtree) {
          child_containing_target = child;
        }
      } else if (ts_node__is_relevant(child, include_anonymous)) {
        later_child = child;
        later_child_is_relevant = true;
        break;
      } else if (ts_node__relevant_child_count(child, include_anonymous) > 0) {
        later_child = child;
        later_child_is_relevant = false;
        break;
      }
    }

    if (child_containing_target.subtree) {
      if (later_child.subtree) {
        later_node = later_child;
        later_node_is_relevant = later_child_is_relevant;
      }
      node = child_containing_target;
    } else if (later_child_is_relevant) {
      return later_child;
    } else if (later_child.subtree) {
      node = later_child;
    } else if (later_node_is_relevant) {
      return later_node;
    } else {
      node = later_node;
    }
  }

  return ts_node__null();
}

static inline bool point_gt(TSPoint a, TSPoint b) {
  return a.row > b.row || (a.row == b.row && a.column > b.column);
}

static inline TSNode ts_node__first_child_for_byte(TSNode self, uint32_t goal,
                                                   bool include_anonymous) {
  TSNode node = self;
  bool did_descend = true;

  while (did_descend) {
    did_descend = false;

    TSNode child;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&node);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      if (ts_node_end_byte(child) > goal) {
        if (ts_node__is_relevant(child, include_anonymous)) {
          return child;
        } else if (ts_node_child_count(child) > 0) {
          did_descend = true;
          node = child;
          break;
        }
      }
    }
  }

  return ts_node__null();
}

static inline TSNode ts_node__descendant_for_byte_range(TSNode self, uint32_t min,
                                                        uint32_t max,
                                                        bool include_anonymous) {
  TSNode node = self;
  TSNode last_visible_node = self;

  bool did_descend = true;
  while (did_descend) {
    did_descend = false;

    TSNode child;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&node);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      if (iterator.position.bytes > max) {
        if (child.byte > min) break;
        node = child;
        if (ts_node__is_relevant(node, include_anonymous)) last_visible_node = node;
        did_descend = true;
        break;
      }
    }
  }

  return last_visible_node;
}

static inline TSNode ts_node__descendant_for_point_range(TSNode self, TSPoint min,
                                                         TSPoint max,
                                                         bool include_anonymous) {
  TSNode node = self;
  TSNode last_visible_node = self;
  TSPoint start_position = ts_node_start_point(self);
  TSPoint end_position = ts_node_end_point(self);

  bool did_descend = true;
  while (did_descend) {
    did_descend = false;

    TSNode child;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&node);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      const Subtree *child_tree = ts_node__tree(child);
      if (iterator.child_index != 1) {
        start_position = point_add(start_position, child_tree->padding.extent);
      }
      end_position = point_add(start_position, child_tree->size.extent);
      if (point_gt(end_position, max)) {
        if (point_gt(start_position, min)) break;
        node = child;
        if (ts_node__is_relevant(node, include_anonymous)) last_visible_node = node;
        did_descend = true;
        break;
      }
      start_position = end_position;
    }
  }

  return last_visible_node;
}

// TSNode - Public

uint32_t ts_node_start_byte(TSNode self) {
  return self.byte + ts_node__tree(self)->padding.bytes;
}

uint32_t ts_node_end_byte(TSNode self) {
  return ts_node_start_byte(self) + ts_node__tree(self)->size.bytes;
}

TSPoint ts_node_start_point(TSNode self) {
  return point_add(self.position, ts_node__tree(self)->padding.extent);
}

TSPoint ts_node_end_point(TSNode self) {
  return point_add(ts_node_start_point(self), ts_node__tree(self)->size.extent);
}

TSSymbol ts_node_symbol(TSNode self) {
  const Subtree *tree = ts_node__tree(self);
  return self.alias_symbol ? self.alias_symbol : tree->symbol;
}

const char *ts_node_type(TSNode self) {
  return ts_language_symbol_name(self.tree->language, ts_node_symbol(self));
}

char *ts_node_string(TSNode self) {
  return ts_subtree_string(ts_node__tree(self), self.tree->language, false);
}

bool ts_node_eq(TSNode self, TSNode other) {
  return (
    ts_subtree_eq(ts_node__tree(self), ts_node__tree(other)) &&
    self.byte == other.byte
  );
}

bool ts_node_is_named(TSNode self) {
  const Subtree *tree = ts_node__tree(self);
  return self.alias_symbol
    ? ts_language_symbol_metadata(self.tree->language, self.alias_symbol).named
    : tree->named;
}

bool ts_node_is_missing(TSNode self) {
  const Subtree *tree = ts_node__tree(self);
  return tree->is_missing;
}

bool ts_node_has_changes(TSNode self) {
  return ts_node__tree(self)->has_changes;
}

bool ts_node_has_error(TSNode self) {
  return ts_node__tree(self)->error_cost > 0;
}

TSNode ts_node_parent(TSNode self) {
  TSNode node = ts_tree_root_node(self.tree);
  uint32_t end_byte = ts_node_end_byte(self);
  if (node.subtree == self.subtree) return ts_node__null();

  TSNode last_visible_node = node;
  bool did_descend = true;
  while (did_descend) {
    did_descend = false;

    TSNode child;
    NodeChildIterator iterator = ts_node_child_iterator_begin(&node);
    while (ts_node_child_iterator_next(&iterator, &child)) {
      if (child.byte > self.byte || child.subtree == self.subtree) break;
      if (iterator.position.bytes >= end_byte) {
        node = child;
        if (ts_node__is_relevant(child, true)) {
          last_visible_node = node;
        }
        did_descend = true;
        break;
      }
    }
  }

  return last_visible_node;
}

TSNode ts_node_child(TSNode self, uint32_t child_index) {
  return ts_node__child(self, child_index, true);
}

TSNode ts_node_named_child(TSNode self, uint32_t child_index) {
  return ts_node__child(self, child_index, false);
}

uint32_t ts_node_child_count(TSNode self) {
  const Subtree *tree = ts_node__tree(self);
  if (tree->children.size > 0) {
    return tree->visible_child_count;
  } else {
    return 0;
  }
}

uint32_t ts_node_named_child_count(TSNode self) {
  const Subtree *tree = ts_node__tree(self);
  if (tree->children.size > 0) {
    return tree->named_child_count;
  } else {
    return 0;
  }
}

TSNode ts_node_next_sibling(TSNode self) {
  return ts_node__next_sibling(self, true);
}

TSNode ts_node_next_named_sibling(TSNode self) {
  return ts_node__next_sibling(self, false);
}

TSNode ts_node_prev_sibling(TSNode self) {
  return ts_node__prev_sibling(self, true);
}

TSNode ts_node_prev_named_sibling(TSNode self) {
  return ts_node__prev_sibling(self, false);
}

TSNode ts_node_first_child_for_byte(TSNode self, uint32_t byte) {
  return ts_node__first_child_for_byte(self, byte, true);
}

TSNode ts_node_first_named_child_for_byte(TSNode self, uint32_t byte) {
  return ts_node__first_child_for_byte(self, byte, false);
}

TSNode ts_node_descendant_for_byte_range(TSNode self, uint32_t min, uint32_t max) {
  return ts_node__descendant_for_byte_range(self, min, max, true);
}

TSNode ts_node_named_descendant_for_byte_range(TSNode self, uint32_t min,
                                               uint32_t max) {
  return ts_node__descendant_for_byte_range(self, min, max, false);
}

TSNode ts_node_descendant_for_point_range(TSNode self, TSPoint min, TSPoint max) {
  return ts_node__descendant_for_point_range(self, min, max, true);
}

TSNode ts_node_named_descendant_for_point_range(TSNode self, TSPoint min,
                                                TSPoint max) {
  return ts_node__descendant_for_point_range(self, min, max, false);
}
