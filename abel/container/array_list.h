//
// Created by liyinbin on 2020/1/31.
//

#ifndef ABEL_CONTAINER_ARRAY_LIST_H_
#define ABEL_CONTAINER_ARRAY_LIST_H_

#include <memory>
#include <algorithm>

namespace abel {

template<typename T, size_t items_per_chunk = 128>
class array_list {
    static_assert((items_per_chunk & (items_per_chunk - 1)) == 0,
                  "array_list chunk size must be power of two");
    union maybe_item {
        maybe_item () noexcept { }
        ~maybe_item () { }
        T data;
    };
    struct chunk {
        maybe_item items[items_per_chunk];
        struct chunk *next;
        // begin and end interpreted mod items_per_chunk
        unsigned begin;
        unsigned end;
    };
    // We pop from the chunk at _front_chunk. This chunk is then linked to
    // the following chunks via the "next" link. _back_chunk points to the
    // last chunk in this list, and it is where we push.
    chunk *_front_chunk = nullptr; // where we pop
    chunk *_back_chunk = nullptr; // where we push
    // We want an O(1) size but don't want to maintain a size() counter
    // because this will slow down every push and pop operation just for
    // the rare size() call. Instead, we just keep a count of chunks (which
    // doesn't change on every push or pop), from which we can calculate
    // size() when needed, and still be O(1).
    // This assumes the invariant that all middle chunks (except the front
    // and back) are always full.
    size_t _nchunks = 0;
    // A list of freed chunks, to support reserve() and to improve
    // performance of repeated push and pop, especially on an empty queue.
    // It is a performance/memory tradeoff how many freed chunks to keep
    // here (see save_free_chunks constant below).
    chunk *_free_chunks = nullptr;
    size_t _nfree_chunks = 0;
public:
    using value_type = T;
    using size_type = size_t;
    using reference = T &;
    using pointer = T *;
    using const_reference = const T &;
    using const_pointer = const T *;

private:
    template<typename U>
    class basic_iterator {
        friend class array_list;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = U;
        using pointer = U *;
        using reference = U &;

    protected:
        chunk *_chunk = nullptr;
        size_t _item_index = 0;

    protected:
        inline explicit basic_iterator (chunk *c);
        inline basic_iterator (chunk *c, size_t item_index);

    public:
        inline bool operator == (const basic_iterator &o) const;
        inline bool operator != (const basic_iterator &o) const;
        inline pointer operator -> () const;
        inline reference operator * () const;
        inline basic_iterator operator ++ (int);
        basic_iterator &operator ++ ();
    };

public:
    class iterator : public basic_iterator<T> {
        using basic_iterator<T>::basic_iterator;
    public:
        iterator () = default;
    };
    class const_iterator : public basic_iterator<const T> {
        using basic_iterator<T>::basic_iterator;
    public:
        const_iterator () = default;
        inline const_iterator (iterator o);
    };

public:
    array_list () = default;
    array_list (array_list &&x) noexcept;
    array_list (const array_list &X) = delete;
    ~array_list ();
    array_list &operator = (const array_list &) = delete;
    array_list &operator = (array_list &&) noexcept;
    inline void push_back (const T &data);
    inline void push_back (T &&data);
    T &back ();
    const T &back () const;
    template<typename... A>
    inline void emplace_back (A &&... args);
    inline T &front () const noexcept;
    inline void pop_front () noexcept;
    inline bool empty () const noexcept;
    inline size_t size () const noexcept;
    void clear () noexcept;
    // reserve(n) ensures that at least (n - size()) further push() calls can
    // be served without needing new memory allocation.
    // Calling pop()s between these push()es is also allowed and does not
    // alter this guarantee.
    // Note that reserve() does not reduce the amount of memory already
    // reserved - use shrink_to_fit() for that.
    void reserve (size_t n);
    // shrink_to_fit() frees memory held, but unused, by the queue. Such
    // unused memory might exist after pops, or because of reserve().
    void shrink_to_fit ();
    inline iterator begin ();
    inline iterator end ();
    inline const_iterator begin () const;
    inline const_iterator end () const;
    inline const_iterator cbegin () const;
    inline const_iterator cend () const;
private:
    void back_chunk_new ();
    void front_chunk_delete () noexcept;
    inline void ensure_room_back ();
    void undo_room_back ();
    static inline size_t mask (size_t idx) noexcept;

};

template<typename T, size_t items_per_chunk>
template<typename U>
inline
array_list<T, items_per_chunk>::basic_iterator<U>::basic_iterator (chunk *c)
    : _chunk(c), _item_index(_chunk ? _chunk->begin : 0) {
}

template<typename T, size_t items_per_chunk>
template<typename U>
inline
array_list<T, items_per_chunk>::basic_iterator<U>::basic_iterator (chunk *c, size_t item_index)
    : _chunk(c), _item_index(item_index) {
}

template<typename T, size_t items_per_chunk>
template<typename U>
inline bool
array_list<T, items_per_chunk>::basic_iterator<U>::operator == (const basic_iterator &o) const {
    return _chunk == o._chunk && _item_index == o._item_index;
}

template<typename T, size_t items_per_chunk>
template<typename U>
inline bool
array_list<T, items_per_chunk>::basic_iterator<U>::operator != (const basic_iterator &o) const {
    return !(*this == o);
}

template<typename T, size_t items_per_chunk>
template<typename U>
inline typename array_list<T, items_per_chunk>::template basic_iterator<U>::pointer
array_list<T, items_per_chunk>::basic_iterator<U>::operator -> () const {
    return &_chunk->items[array_list::mask(_item_index)].data;
}

template<typename T, size_t items_per_chunk>
template<typename U>
inline typename array_list<T, items_per_chunk>::template basic_iterator<U>::reference
array_list<T, items_per_chunk>::basic_iterator<U>::operator * () const {
    return _chunk->items[array_list::mask(_item_index)].data;
}

template<typename T, size_t items_per_chunk>
template<typename U>
inline typename array_list<T, items_per_chunk>::template basic_iterator<U>
array_list<T, items_per_chunk>::basic_iterator<U>::operator ++ (int) {
    auto it = *this;
    ++(*this);
    return it;
}

template<typename T, size_t items_per_chunk>
template<typename U>
typename array_list<T, items_per_chunk>::template basic_iterator<U> &
array_list<T, items_per_chunk>::basic_iterator<U>::operator ++ () {
    ++_item_index;
    if (_item_index == _chunk->end) {
        _chunk = _chunk->next;
        _item_index = _chunk ? _chunk->begin : 0;
    }
    return *this;
}

template<typename T, size_t items_per_chunk>
inline
array_list<T, items_per_chunk>::const_iterator::const_iterator (array_list<T, items_per_chunk>::iterator o)
    : basic_iterator<const T>(o._chunk, o._item_index) {
}

template<typename T, size_t items_per_chunk>
inline
array_list<T, items_per_chunk>::array_list (array_list &&x) noexcept
    : _front_chunk(x._front_chunk), _back_chunk(x._back_chunk), _nchunks(x._nchunks), _free_chunks(x._free_chunks),
      _nfree_chunks(x._nfree_chunks) {
    x._front_chunk = nullptr;
    x._back_chunk = nullptr;
    x._nchunks = 0;
    x._free_chunks = nullptr;
    x._nfree_chunks = 0;
}

template<typename T, size_t items_per_chunk>
inline
array_list<T, items_per_chunk> &
array_list<T, items_per_chunk>::operator = (array_list &&x) noexcept {
    if (&x != this) {
        this->~array_list();
        new(this) array_list(std::move(x));
    }
    return *this;
}

template<typename T, size_t items_per_chunk>
inline size_t
array_list<T, items_per_chunk>::mask (size_t idx) noexcept {
    return idx & (items_per_chunk - 1);
}

template<typename T, size_t items_per_chunk>
inline bool
array_list<T, items_per_chunk>::empty () const noexcept {
    return _front_chunk == nullptr;
}

template<typename T, size_t items_per_chunk>
inline size_t
array_list<T, items_per_chunk>::size () const noexcept {
    if (_front_chunk == nullptr) {
        return 0;
    } else if (_back_chunk == _front_chunk) {
        // Single chunk.
        return _front_chunk->end - _front_chunk->begin;
    } else {
        return _front_chunk->end - _front_chunk->begin
            + _back_chunk->end - _back_chunk->begin
            + (_nchunks - 2) * items_per_chunk;
    }
}

template<typename T, size_t items_per_chunk>
void array_list<T, items_per_chunk>::clear () noexcept {
#if 1
    while (!empty()) {
        pop_front();
    }
#else
    // This is specialized code to free the contents of all the chunks and the
    // chunks themselves. but since destroying a very full queue is not an
    // important use case to optimize, the simple loop above is preferable.
    if (!_front_chunk) {
        // Empty, nothing to do
        return;
    }
    // Delete front chunk (partially filled)
    for (auto i = _front_chunk->begin; i != _front_chunk->end; ++i) {
        _front_chunk->items[mask(i)].data.~T();
    }
    chunk *p = _front_chunk->next;
    delete _front_chunk;
    // Delete all the middle chunks (all completely filled)
    if (p) {
        while (p != _back_chunk) {
            // These are full chunks
            chunk *nextp = p->next;
            for (auto i = 0; i != items_per_chunk; ++i) {
                // Note we delete out of order (we don't start with p->begin).
                // That should be fine..
                p->items[i].data.~T();
        }
            delete p;
            p = nextp;
        }
        // Finally delete back chunk (partially filled)
        for (auto i = _back_chunk->begin; i != _back_chunk->end; ++i) {
            _back_chunk->items[mask(i)].data.~T();
        }
        delete _back_chunk;
    }
    _front_chunk = nullptr;
    _back_chunk = nullptr;
    _nchunks = 0;
#endif
}

template<typename T, size_t items_per_chunk> void
array_list<T, items_per_chunk>::shrink_to_fit () {
    while (_free_chunks) {
        auto next = _free_chunks->next;
        delete _free_chunks;
        _free_chunks = next;
    }
    _nfree_chunks = 0;
}

template<typename T, size_t items_per_chunk>
array_list<T, items_per_chunk>::~array_list () {
    clear();
    shrink_to_fit();
}

template<typename T, size_t items_per_chunk>
void
array_list<T, items_per_chunk>::back_chunk_new () {
    chunk *old = _back_chunk;
    if (_free_chunks) {
        _back_chunk = _free_chunks;
        _free_chunks = _free_chunks->next;
        --_nfree_chunks;
    } else {
        _back_chunk = new chunk;
    }
    _back_chunk->next = nullptr;
    _back_chunk->begin = 0;
    _back_chunk->end = 0;
    if (old) {
        old->next = _back_chunk;
    }
    if (_front_chunk == nullptr) {
        _front_chunk = _back_chunk;
    }
    _nchunks++;
}

template<typename T, size_t items_per_chunk>
inline void
array_list<T, items_per_chunk>::ensure_room_back () {
    // If we don't have a back chunk or it's full, we need to create a new one
    if (_back_chunk == nullptr ||
        (_back_chunk->end - _back_chunk->begin) == items_per_chunk) {
        back_chunk_new();
    }
}

template<typename T, size_t items_per_chunk>
void
array_list<T, items_per_chunk>::undo_room_back () {
    // If we failed creating a new item after ensure_room_back() created a
    // new empty chunk, we must remove it, or empty() will be incorrect
    // (either immediately, if the fifo was empty, or when all the items are
    // popped, if it already had items).
    if (_back_chunk->begin == _back_chunk->end) {
        delete _back_chunk;
        --_nchunks;
        if (_nchunks == 0) {
            _back_chunk = nullptr;
            _front_chunk = nullptr;
        } else {
            // Because we don't usually pop from the back, we don't have a "prev"
            // pointer so we need to find the previous chunk the hard and slow
            // way. B
            chunk *old = _back_chunk;
            _back_chunk = _front_chunk;
            while (_back_chunk->next != old) {
                _back_chunk = _back_chunk->next;
            }
            _back_chunk->next = nullptr;
        }
    }

}

template<typename T, size_t items_per_chunk>
template<typename... Args>
inline void
array_list<T, items_per_chunk>::emplace_back (Args &&... args) {
    ensure_room_back();
    auto p = &_back_chunk->items[mask(_back_chunk->end)].data;
    try {
        new(p) T(std::forward<Args>(args)...);
    } catch (...) {
        undo_room_back();
        throw;
    }
    ++_back_chunk->end;
}

template<typename T, size_t items_per_chunk>
inline void
array_list<T, items_per_chunk>::push_back (const T &data) {
    ensure_room_back();
    auto p = &_back_chunk->items[mask(_back_chunk->end)].data;
    try {
        new(p) T(data);
    } catch (...) {
        undo_room_back();
        throw;
    }
    ++_back_chunk->end;
}

template<typename T, size_t items_per_chunk>
inline void
array_list<T, items_per_chunk>::push_back (T &&data) {
    ensure_room_back();
    auto p = &_back_chunk->items[mask(_back_chunk->end)].data;
    try {
        new(p) T(std::move(data));
    } catch (...) {
        undo_room_back();
        throw;
    }
    ++_back_chunk->end;
}

template<typename T, size_t items_per_chunk>
inline
T &
array_list<T, items_per_chunk>::back () {
    return _back_chunk->items[mask(_back_chunk->end - 1)].data;
}

template<typename T, size_t items_per_chunk>
inline
const T &
array_list<T, items_per_chunk>::back () const {
    return _back_chunk->items[mask(_back_chunk->end - 1)].data;
}

template<typename T, size_t items_per_chunk>
inline T &
array_list<T, items_per_chunk>::front () const noexcept {
    return _front_chunk->items[mask(_front_chunk->begin)].data;
}

template<typename T, size_t items_per_chunk>
inline void
array_list<T, items_per_chunk>::front_chunk_delete () noexcept {
    chunk *next = _front_chunk->next;
    // Certain use cases may need to repeatedly allocate and free a chunk -
    // an obvious example is an empty queue to which we push, and then pop,
    // repeatedly. Another example is pushing and popping to a non-empty queue
    // we push and pop at different chunks so we need to free and allocate a
    // chunk every items_per_chunk operations.
    // The solution is to keep a list of freed chunks instead of freeing them
    // immediately. There is a performance/memory tradeoff of how many freed
    // chunks to save: If we save them all, the queue can never shrink from
    // its maximum memory use (this is how circular_buffer behaves).
    // The ad-hoc choice made here is to limit the number of saved chunks to 1,
    // but this could easily be made a configuration option.
    static constexpr int save_free_chunks = 1;
    if (_nfree_chunks < save_free_chunks) {
        _front_chunk->next = _free_chunks;
        _free_chunks = _front_chunk;
        ++_nfree_chunks;
    } else {
        delete _front_chunk;
    }
    // If we only had one chunk, _back_chunk is gone too.
    if (_back_chunk == _front_chunk) {
        _back_chunk = nullptr;
    }
    _front_chunk = next;
    --_nchunks;
}

template<typename T, size_t items_per_chunk>
inline void
array_list<T, items_per_chunk>::pop_front () noexcept {
    front().~T();
    // If the front chunk has become empty, we need to free remove it and use
    // the next one.
    if (++_front_chunk->begin == _front_chunk->end) {
        front_chunk_delete();
    }
}

template<typename T, size_t items_per_chunk>
void array_list<T, items_per_chunk>::reserve (size_t n) {
    // reserve() guarantees that (n - size()) additional push()es will
    // succeed without reallocation:
    size_t need = n - size();
    // If we already have a back chunk, it might have room for some pushes
    // before filling up, so decrease "need":
    if (_back_chunk) {
        need -= items_per_chunk - (_back_chunk->end - _back_chunk->begin);
    }
    size_t needed_chunks = (need + items_per_chunk - 1) / items_per_chunk;
    // If we already have some freed chunks saved, we need to allocate fewer
    // additional chunks, or none at all
    if (needed_chunks <= _nfree_chunks) {
        return;
    }
    needed_chunks -= _nfree_chunks;
    while (needed_chunks--) {
        chunk *c = new chunk;
        c->next = _free_chunks;
        _free_chunks = c;
        ++_nfree_chunks;
    }
}

template<typename T, size_t items_per_chunk>
inline typename array_list<T, items_per_chunk>::iterator
array_list<T, items_per_chunk>::begin () {
    return iterator(_front_chunk);
}

template<typename T, size_t items_per_chunk>
inline typename array_list<T, items_per_chunk>::iterator
array_list<T, items_per_chunk>::end () {
    return iterator(nullptr);
}

template<typename T, size_t items_per_chunk>
inline typename array_list<T, items_per_chunk>::const_iterator
array_list<T, items_per_chunk>::begin () const {
    return const_iterator(_front_chunk);
}

template<typename T, size_t items_per_chunk>
inline typename array_list<T, items_per_chunk>::const_iterator
array_list<T, items_per_chunk>::end () const {
    return const_iterator(nullptr);
}

template<typename T, size_t items_per_chunk>
inline typename array_list<T, items_per_chunk>::const_iterator
array_list<T, items_per_chunk>::cbegin () const {
    return const_iterator(_front_chunk);
}

template<typename T, size_t items_per_chunk>
inline typename array_list<T, items_per_chunk>::const_iterator
array_list<T, items_per_chunk>::cend () const {
    return const_iterator(nullptr);
}

} //namespace abel

#endif //ABEL_CONTAINER_ARRAY_LIST_H_
