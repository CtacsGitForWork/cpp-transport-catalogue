#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json.h"
#include <iostream>

int main() {
    json::Document doc = json::Load(std::cin);
    const json::Node& root = doc.GetRoot();

    transport_catalogue::TransportCatalogue db;
    JsonReader reader(db);

    // Настройки
    reader.ParseBaseRequests(root);
    auto render_settings = reader.ParseRenderSettings(root);
    auto routing_settings = reader.ParseRoutingSettings(root);
    reader.ParseStatRequests(root);

    // Парсим запросы
    reader.ParseBaseRequests(root);
    reader.ParseStatRequests(root);

    // Создаем рендерер и обработчик запросов
    map_renderer::MapRenderer renderer(render_settings);
    TransportRouter router(db, routing_settings);
    RequestHandler handler(db, renderer, &router);

    // Обрабатываем запросы (включая рендеринг карты)
    auto answers = reader.ProcessStatRequests(handler);
    json::Print(json::Document{ answers }, std::cout);
}
