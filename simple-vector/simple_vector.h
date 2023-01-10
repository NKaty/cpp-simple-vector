#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>

#include "array_ptr.h"

class ReserveProxyObj {
 public:
  explicit ReserveProxyObj(size_t size) : size_(size) {
  }

  size_t GetReservedCapacity() const noexcept {
    return size_;
  }

 private:
  size_t size_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
  return ReserveProxyObj(capacity_to_reserve);
}

template<typename Type>
class SimpleVector {
 public:
  using Iterator = Type *;
  using ConstIterator = const Type *;

  SimpleVector() noexcept = default;

  // Создаёт вектор из size элементов, инициализированных значением по умолчанию
  explicit SimpleVector(size_t size) : array_(size), size_(size), capacity_(size) {
    std::generate(begin(), end(), []() { return Type{}; });
  }

  explicit SimpleVector(ReserveProxyObj reserve_proxy_obj)
      : capacity_(reserve_proxy_obj.GetReservedCapacity()) {

  }

  // Создаёт вектор из size элементов, инициализированных значением value
  SimpleVector(size_t size, const Type &value)
      : array_(size), size_(size), capacity_(size) {
    std::fill(begin(), end(), value);
  }

  // Создаёт вектор из std::initializer_list
  SimpleVector(std::initializer_list<Type> init)
      : array_(init.size()), size_(init.size()), capacity_(init.size()) {
    std::copy(init.begin(), init.end(), array_.Get());
  }

  SimpleVector(const SimpleVector &other) {
    SimpleVector copy(other.size_);
    std::copy(other.begin(), other.end(), copy.begin());
    swap(copy);
  }

  SimpleVector(SimpleVector &&other) noexcept {
    array_.exchange(other.array_);
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.Clear();
  }

  SimpleVector &operator=(const SimpleVector &rhs) {
    if (this == &rhs) {
      return *this;
    }
    SimpleVector copy(rhs);
    swap(copy);
    return *this;
  }

  SimpleVector &operator=(SimpleVector &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    array_.exchange(rhs.array_);
    size_ = rhs.size_;
    capacity_ = rhs.capacity_;
    rhs.Clear();
    return *this;
  }

  // Возвращает количество элементов в массиве
  size_t GetSize() const noexcept {
    return size_;
  }

  // Возвращает вместимость массива
  size_t GetCapacity() const noexcept {
    return capacity_;
  }

  // Сообщает, пустой ли массив
  bool IsEmpty() const noexcept {
    return size_ == 0;
  }

  // Возвращает ссылку на элемент с индексом index
  Type &operator[](size_t index) noexcept {
    return array_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  const Type &operator[](size_t index) const noexcept {
    return array_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  Type &At(size_t index) {
    if (index >= size_) {
      throw std::out_of_range("out_of_range");
    }
    return array_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  const Type &At(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("out_of_range");
    }
    return array_[index];
  }

  // Обнуляет размер массива, не изменяя его вместимость
  void Clear() noexcept {
    size_ = 0;
  }

  // Изменяет размер массива.
  // При увеличении размера новые элементы получают значение по умолчанию для типа Type
  void Resize(size_t new_size) {
    if (new_size < size_) {
      size_ = new_size;
      return;
    }
    if (new_size > size_ && new_size <= capacity_) {
      std::generate(begin() + size_, begin() + new_size, []() { return Type{}; });
      size_ = new_size;
      return;
    }
    if (new_size > capacity_) {
      auto new_capacity = std::max(new_size, size_ * 2);
      ArrayPtr<Type> new_array(new_capacity);
      std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), new_array.Get());
      std::generate(new_array.Get() + size_, new_array.Get() + new_size, []() { return Type{}; });
      array_.swap(new_array);
      size_ = new_size;
      capacity_ = new_capacity;
    }
  }

  // Добавляет элемент в конец вектора
  // При нехватке места увеличивает вдвое вместимость вектора
  void PushBack(const Type &item) {
    Resize(size_ + 1);
    array_[size_ - 1] = item;
  }

  void PushBack(Type &&item) {
    Resize(size_ + 1);
    array_[size_ - 1] = std::move(item);
  }

  // Вставляет значение value в позицию pos.
  // Возвращает итератор на вставленное значение
  // Если перед вставкой значения вектор был заполнен полностью,
  // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
  Iterator Insert(ConstIterator pos, const Type &value) {
    auto index = std::distance(cbegin(), pos);
    Resize(size_ + 1);
    std::copy_backward(cbegin() + index, cend() - 1, end());
    array_[index] = value;
    return Iterator(&array_[index]);
  }

  Iterator Insert(ConstIterator pos, Type &&value) {
    auto index = std::distance(cbegin(), pos);
    Resize(size_ + 1);
    std::copy_backward(std::make_move_iterator(begin() + index),
                       std::make_move_iterator(end() - 1),
                       end());
    array_[index] = std::move(value);
    return Iterator(&array_[index]);
  }

  // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
  void PopBack() noexcept {
    if (IsEmpty()) {
      return;
    }
    size_ -= 1;
  }

  // Удаляет элемент вектора в указанной позиции
  Iterator Erase(ConstIterator pos) {
    assert(pos >= begin() && pos < end());
    auto index = std::distance(cbegin(), pos);
    std::copy(std::make_move_iterator(begin() + index + 1),
              std::make_move_iterator(end()),
              &array_[index]);
    --size_;
    return Iterator(&array_[index]);
  }

  void Reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
      ArrayPtr<Type> new_array(new_capacity);
      std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), new_array.Get());
      array_.swap(new_array);
      capacity_ = new_capacity;
    }
  }

  // Обменивает значение с другим вектором
  void swap(SimpleVector &other) noexcept {
    array_.swap(other.array_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

  // Возвращает итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator begin() noexcept {
    return Iterator(&array_[0]);
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator end() noexcept {
    return Iterator(&array_[size_]);
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator begin() const noexcept {
    return ConstIterator(&array_[0]);
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator end() const noexcept {
    return ConstIterator(&array_[size_]);
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cbegin() const noexcept {
    return ConstIterator(&array_[0]);
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cend() const noexcept {
    return ConstIterator(&array_[size_]);
  }

 private:
  ArrayPtr<Type> array_;
  size_t size_ = 0;
  size_t capacity_ = 0;
};

template<typename Type>
inline bool operator==(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs) {
  return !(lhs > rhs) && !(lhs < rhs);
}

template<typename Type>
inline bool operator!=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs) {
  return !(lhs == rhs);
}

template<typename Type>
inline bool operator<(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs) {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename Type>
inline bool operator<=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs) {
  return !(lhs > rhs);
}

template<typename Type>
inline bool operator>(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs) {
  return rhs < lhs;
}

template<typename Type>
inline bool operator>=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs) {
  return !(lhs < rhs);
}
