#include "map_renderer.h"
#include <algorithm>

namespace map_renderer {

    using namespace std::literals;

    MapRenderer::MapRenderer(const RenderSettings& settings)
        : settings_(settings) {
    }

    svg::Document MapRenderer::Render(
        const std::vector<const domain::Bus*>& buses,
        const std::vector<const domain::Stop*>& stops,
        const SphereProjector& projector) const
    {
        svg::Document doc;

        // Отрисовываем в правильном порядке
        RenderBusLines(doc, buses, projector);        // 1. Линии маршрутов
        RenderBusLabels(doc, buses, projector);       // 2. Названия маршрутов
        RenderStopPoints(doc, stops, projector);      // 3. Круги остановок
        RenderStopLabels(doc, stops, projector);      // 4. Названия остановок

        return doc;
    }

    void MapRenderer::RenderBusLines(svg::Document& doc,
        const std::vector<const domain::Bus*>& buses,
        const SphereProjector& projector) const
    {
        size_t color_index = 0;
        for (const auto* bus : buses) {
            if (!bus || bus->stops.empty()) continue;

            svg::Polyline polyline;
            polyline.SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()]);
            polyline.SetFillColor(svg::NoneColor);
            polyline.SetStrokeWidth(settings_.line_width);
            polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            // Добавляем точки маршрута
            for (const auto* stop : bus->stops) {
                polyline.AddPoint(projector(stop->coordinates));
            }

            // Для некольцевых маршрутов добавляем обратный путь (кроме последней остановки)
            if (!bus->is_roundtrip) {
                for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                    polyline.AddPoint(projector((*it)->coordinates));
                }
            }

            doc.Add(std::move(polyline));
            ++color_index;
        }
    }

    void MapRenderer::RenderBusLabels(svg::Document& doc,
        const std::vector<const domain::Bus*>& buses,
        const SphereProjector& projector) const
    {
        size_t color_index = 0;
        for (const auto* bus : buses) {
            if (!bus || bus->stops.empty()) continue;

            const auto color = settings_.color_palette[color_index % settings_.color_palette.size()];

            // Функция для отрисовки одной метки
            auto render_label = [&](const domain::Stop* stop) {
                const svg::Point pos = projector(stop->coordinates);
                // Подложка
                svg::Text underlayer;
                underlayer.SetPosition(pos)
                    .SetOffset(settings_.bus_label_offset)
                    .SetFontSize(settings_.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(bus->name)
                    .SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color)
                    .SetStrokeWidth(settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
                doc.Add(std::move(underlayer));

                // Основной текст
                svg::Text label;
                label.SetPosition(pos)
                    .SetOffset(settings_.bus_label_offset)
                    .SetFontSize(settings_.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(bus->name)
                    .SetFillColor(color);
                doc.Add(std::move(label));
                };

            // Первая метка
            render_label(bus->stops.front());

            // Вторая метка для некольцевого маршрута
            if (!bus->is_roundtrip && bus->stops.front() != bus->stops.back()) {
                render_label(bus->stops.back());
            }

            ++color_index;
        }
    }

    void MapRenderer::RenderStopPoints(svg::Document& doc,
        const std::vector<const domain::Stop*>& stops,
        const SphereProjector& projector) const
    {
        for (const auto* stop : stops) {
            svg::Circle circle;
            circle.SetCenter(projector(stop->coordinates));
            circle.SetRadius(settings_.stop_radius);
            circle.SetFillColor("white");
            doc.Add(std::move(circle));
        }
    }

    void MapRenderer::RenderStopLabels(svg::Document& doc,
        const std::vector<const domain::Stop*>& stops,
        const SphereProjector& projector) const
    {
        for (const auto* stop : stops) {
            svg::Point pos = projector(stop->coordinates);

            // Подложка текста
            svg::Text underlayer;
            underlayer.SetPosition(pos)
                .SetOffset(settings_.stop_label_offset)
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop->name)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(std::move(underlayer));

            // Основной текст
            svg::Text label;
            label.SetPosition(pos)
                .SetOffset(settings_.stop_label_offset)
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop->name)
                .SetFillColor("black");
            doc.Add(std::move(label));
        }
    }

}  // namespace map_renderer
