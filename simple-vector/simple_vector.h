#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <iterator>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj() = default;
    ReserveProxyObj(const size_t value)
        : capacity_to_reserve_(value)
    {
    }

    size_t GetCapacity() const {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_ = 0;
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : items_(size), size_(size), capacity_(size)
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : items_(size), size_(size), capacity_(size)
    {
        if (size == 0) { return; }

        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size()), size_(init.size()), capacity_(init.size())
    {

        //assert(size_ == 0);
        std::copy(init.begin(), init.end(), items_.Get());

    }

    // Конструктор копирования
    SimpleVector(const SimpleVector& other)
        : items_(other.GetCapacity()), size_(other.GetSize()), capacity_(other.GetCapacity())
    {

        //assert(size_ == 0 && capacity_ == 0);
        std::copy(other.begin(), other.end(), items_.Get());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {

        const SimpleVector* rhs_prt = &rhs;
        if (this == rhs_prt) { return *this; }

        SimpleVector<Type> tmp{ rhs };

        swap(tmp);

        return *this;
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& other) {

        items_ = std::move(other.items_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);

    }

    // Оператор перемещения
    SimpleVector& operator=(SimpleVector&& rhs) {

        const SimpleVector* rhs_prt = &rhs;
        if (this == rhs_prt) { return *this; }

        items_ = std::move(rhs.items_);
        size_ = std::exchange(rhs.size_, 0);
        capacity_ = std::exchange(rhs.capacity_, 0);

        return *this;
    }

    SimpleVector(ReserveProxyObj rhs) {
        Reserve(rhs.GetCapacity());
    };

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {

        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);

    }

    // Вставляет элемент в конец вектора
    void PushBack(const Type& item) {

        CheckCapacityAndResize();
        items_[size_ - 1] = item;

    }

    // Вставляет элемент в конец вектора, перемещением
    void PushBack(Type&& item) {

        CheckCapacityAndResize();
        items_[size_ - 1] = std::move(item);

    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        // Проверка диапазона
        assert(pos >= cbegin() && pos <= cend());
        
        // вставка в конец
        if (pos == cend()) {
            PushBack(value);
            return (end() - 1);
        }

        size_t curr_index = pos - items_.Get();

        // вставка влюбое место, кроме конца        
        CheckCapacityAndResize();

        Iterator it = std::copy_backward(&items_[curr_index], (end() - 1), end());
        --it;
        *it = value;

        return it;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        // вставка в конец
        if (pos == cend()) {
            PushBack(std::move(value));
            return (end() - 1);
        }

        // т.к. указатели и ссылки при вставке сбрасываются, определим текущий индекс до изменения.
        size_t curr_index = pos - items_.Get();

        // вставка влюбое место, кроме конца
        CheckCapacityAndResize();

        Iterator it = std::move_backward(&items_[curr_index], (end() - 1), end());
        --it;
        *it = std::move(value);

        return it;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        // Проверка диапазона
        assert(pos >= begin() && pos < end());

        size_t curr_index = pos - begin();

        if (pos < (end() - 1)) {
            //std::copy(&items_[(curr_index + 1)], end(), &items_[curr_index]);
            std::move(&items_[(curr_index + 1)], end(), &items_[curr_index]);
        }
        --size_;

        return &items_[curr_index];
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
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("");
        }
        return items_[index];
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {

            size_t new_capacity = std::max(new_size, (capacity_ == 0 ? 1 : capacity_ * 2));
            ArrayPtr<Type> arr_tmp(new_capacity);

            //std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), std::make_move_iterator(arr_tmp.Get()));
            std::move(begin(), end(), arr_tmp.Get());
            items_.swap(arr_tmp);

            size_ = new_size;
            capacity_ = new_capacity;

        }
        else if (new_size > size_) {

            //std::fill(items_.Get() + size_, items_.Get() + new_size, Type());
            for (auto it = (items_.Get() + size_); it < (items_.Get() + new_size); ++it) {
                *it = Type{};
            }

            size_ = new_size;

        }
        else {
            size_ = new_size;
        }
    }

    // Резервирование памяти
    void Reserve(size_t new_capacity) {
        if (capacity_ < new_capacity) {
            ArrayPtr<Type> arr_tmp(new_capacity);

            //std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), std::make_move_iterator(arr_tmp.Get()));
            std::move(begin(), end(), arr_tmp.Get());
            items_.swap(arr_tmp);

            capacity_ = new_capacity;
        }

    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Возвращает итератор на начало массива
        // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;

    // Проверяет размер с емкостью и при необходимости увеличивает емкость
    void CheckCapacityAndResize() {
        if (size_ >= capacity_) {
            Resize(size_ + 1);
        }
        else {
            ++size_;
        }
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}