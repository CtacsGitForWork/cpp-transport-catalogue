#include "json_builder.h"

namespace json {

    Builder::Builder() {
        // Изначально стек содержит nullptr, чтобы различать
        // пустой builder и builder с начатым документом
        nodes_stack_.push_back(nullptr);
    }

    Node Builder::Build() {
        if (nodes_stack_.size() > 1) {
            throw std::logic_error("Building incomplete JSON");
        }
        if (nodes_stack_.front() == nullptr) {
            // Если ничего не добавлено, возвращаем пустой узел
            return Node(nullptr);
        }
        return *nodes_stack_.front();
    }

    Node::Value& Builder::GetCurrentValue() {
        if (nodes_stack_.empty() || !nodes_stack_.back()) {
            throw std::logic_error("No current value available");
        }
        return nodes_stack_.back()->GetValue();
    }

    const Node::Value& Builder::GetCurrentValue() const {
        if (nodes_stack_.empty() || !nodes_stack_.back()) {
            throw std::logic_error("No current value available");
        }
        return nodes_stack_.back()->GetValue();
    }

    Builder::DictValueContext Builder::Key(std::string key) {
        if (nodes_stack_.empty() || !nodes_stack_.back() || !nodes_stack_.back()->IsDict()) {
            throw std::logic_error("Key() called outside of dictionary");
        }

        auto& dict = std::get<Dict>(GetCurrentValue());
        auto [it, inserted] = dict.emplace(std::move(key), nullptr);
        if (!inserted) {
            throw std::logic_error("Duplicate key in dictionary");
        }

        // Сохраняем указатель на значение, которое будет установлено следующим Value()
        nodes_stack_.push_back(&it->second);

        return DictValueContext(*this);
    }

    Builder::BaseContext Builder::Value(Node::Value value) {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Value() called on empty builder");
        }

        // Если стек содержит только nullptr (начальное состояние)
        if (nodes_stack_.size() == 1 && nodes_stack_.back() == nullptr) {
            nodes_stack_.back() = &root_;
            root_ = Node(std::move(value));
            return BaseContext(*this);
        }

        if (!nodes_stack_.back()) {
            throw std::logic_error("Value() called in invalid context");
        }

        if (nodes_stack_.back()->IsArray()) {
            auto& array = std::get<Array>(GetCurrentValue());
            array.emplace_back(std::move(value));
        }
        else if (nodes_stack_.size() > 1) {
            // Устанавливаем значение для последнего элемента в стеке
            *nodes_stack_.back() = Node(std::move(value));
            nodes_stack_.pop_back();
        }
        else {
            throw std::logic_error("Value() called in wrong context");
        }

        return BaseContext(*this);
    }

    Builder::DictItemContext Builder::StartDict() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("StartDict() called on empty builder");
        }

        // Если это первый элемент (стек содержит только nullptr)
        if (nodes_stack_.size() == 1 && nodes_stack_.back() == nullptr) {
            nodes_stack_.back() = &root_;
            root_ = Node(Dict{});
            return DictItemContext(*this);
        }

        if (!nodes_stack_.back()) {
            throw std::logic_error("StartDict() called in invalid context");
        }

        if (nodes_stack_.back()->IsArray()) {
            auto& array = std::get<Array>(GetCurrentValue());
            array.emplace_back(Dict{});
            nodes_stack_.push_back(&array.back());
        }
        else {
            *nodes_stack_.back() = Node(Dict{});
        }

        return DictItemContext(*this);
    }

    Builder::ArrayItemContext Builder::StartArray() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("StartArray() called on empty builder");
        }

        // Если это первый элемент (стек содержит только nullptr)
        if (nodes_stack_.size() == 1 && nodes_stack_.back() == nullptr) {
            nodes_stack_.back() = &root_;
            root_ = Node(Array{});
            return ArrayItemContext(*this);
        }

        if (!nodes_stack_.back()) {
            throw std::logic_error("StartArray() called in invalid context");
        }

        if (nodes_stack_.back()->IsArray()) {
            auto& array = std::get<Array>(GetCurrentValue());
            array.emplace_back(Array{});
            nodes_stack_.push_back(&array.back());
        }
        else {
            *nodes_stack_.back() = Node(Array{});
        }

        return ArrayItemContext(*this);
    }

    Builder::BaseContext Builder::EndDict() {
        if (nodes_stack_.empty() || !nodes_stack_.back() || !nodes_stack_.back()->IsDict()) {
            throw std::logic_error("EndDict() called without matching StartDict()");
        }
       if (nodes_stack_.size() == 1) {
            // Не удаляем корневой элемент, иначе стек станет пуст
            return BaseContext(*this);
        }
        nodes_stack_.pop_back();
        return BaseContext(*this);
    }

    Builder::BaseContext Builder::EndArray() {
        if (nodes_stack_.empty() || !nodes_stack_.back() || !nodes_stack_.back()->IsArray()) {
            throw std::logic_error("EndArray() called without matching StartArray()");
        }
        if (nodes_stack_.size() == 1) {
            // Не удаляем корневой элемент, иначе стек станет пуст
            return BaseContext(*this);
        }
        nodes_stack_.pop_back();
        return BaseContext(*this);
    }

} // namespace json