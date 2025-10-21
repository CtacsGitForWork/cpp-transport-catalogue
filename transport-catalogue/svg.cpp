#include "svg.h"

namespace svg {

using namespace std::literals;

// Вспомогательные функции для вывода цветов
namespace detail {
    void PrintColor(std::ostream& out, std::monostate) {
        out << "none"sv;
    }
    
    void PrintColor(std::ostream& out, const std::string& color) {
        out << color;
    }
    
    void PrintColor(std::ostream& out, Rgb rgb) {
        out << "rgb("sv 
            << static_cast<int>(rgb.red) << ","sv
            << static_cast<int>(rgb.green) << ","sv
            << static_cast<int>(rgb.blue) << ")"sv;
    }
    
    void PrintColor(std::ostream& out, Rgba rgba) {
        out << "rgba("sv 
            << static_cast<int>(rgba.red) << ","sv
            << static_cast<int>(rgba.green) << ","sv
            << static_cast<int>(rgba.blue) << ","sv
            << rgba.opacity << ")"sv;
    }
} // namespace detail

// Оператор вывода для Color
std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit([&out](const auto& value) {
        detail::PrintColor(out, value);
    }, color);
    return out;
}

// Операторы вывода для StrokeLineCap и StrokeLineJoin
std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT: out << "butt"; break;
        case StrokeLineCap::ROUND: out << "round"; break;
        case StrokeLineCap::SQUARE: out << "square"; break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    switch (line_join) {
        case StrokeLineJoin::ARCS: out << "arcs"; break;
        case StrokeLineJoin::BEVEL: out << "bevel"; break;
        case StrokeLineJoin::MITER: out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"; break;
        case StrokeLineJoin::ROUND: out << "round"; break;
    }
    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    // Делегируем вывод тега своим подклассам
    RenderObject(context);
    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\"";
    
    bool first = true;
    for (const Point& point : points_) {
        if (!first) {
            out << " ";
        }
        out << point.x << "," << point.y;
        first = false;
    }
    
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text";

    // Эталонный порядок: fill, stroke, ..., x, y, dx, dy, font-size, font-family, font-weight
    RenderAttrs(out); // ← Сначала отрисовываем стили (fill, stroke, stroke-width и т.п.)

    out << " x=\"" << position_.x << "\" y=\"" << position_.y << "\"";
    out << " dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\"";
    out << " font-size=\"" << font_size_ << "\"";

    if (!font_family_.empty()) {
        out << " font-family=\"" << font_family_ << "\"";
    }

    if (!font_weight_.empty()) {
        out << " font-weight=\"" << font_weight_ << "\"";
    }

    out << ">"sv;

    // XML-экранирование текста
    for (char c : data_) {
        switch (c) {
            case '"': out << "&quot;"; break;
            case '\'': out << "&apos;"; break;
            case '<': out << "&lt;"; break;
            case '>': out << "&gt;"; break;
            case '&': out << "&amp;"; break;
            default: out << c; break;
        }
    }

    out << "</text>";
}


void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    
    RenderContext ctx(out, 2, 2);
    for (const auto& obj : objects_) {
        obj->Render(ctx);
    }
    
    out << "</svg>\n";
}

}  // namespace svg
