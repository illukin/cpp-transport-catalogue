# Транспортный справочник

Программная реализация системы хранения транспортных маршрутов и обработки
запросов к ней. В программе реализованы следующие функции:
1. вычисление длины маршрута по заданным географическим координатам остановок;
2. вычисление извилистости маршрута (она равна отношению длины фактического
расстояния к длине географического расстояния);
3. определение общего количества остановок на маршруте и количества уникальных
остановок;
4. формирование карты маршрутов.

## Структура программы
Программа состоит из нескольких независимых модулей:
1. модуль чтения и обработки запросов в формате JSON;
2. модуль самого транспортного справочника (БД);
3. модуль визуализации карты маршрутов в формате SVG.

Подобная архитектура позволяет гибко модифицировать или вовсе заменить любой из
них, не затрагивая остальные. Например, для работы с форматами JSON и SVG
используются библиотеки собственной разработки, которые при необходимости могут
быть заменены любыми другими с аналогичным функционалом.

Структура проекта отражена на следующей схеме:

![diag](https://user-images.githubusercontent.com/62634576/166454262-1b8c6a30-b671-4e32-ab5e-d68e4aa16bba.svg)

## Использование
Подать на вход программы данные в формате JSON. Каждый поданный объект может
содержать в себе как поля с данными для заполнения транспортного справочника,
так и запросы к нему. Первыми будут обработаны данные заполняющие справочник,
затем будут обработаны запросы и выведен результат.

<details>
  <summary>Описание допустимых полей JSON-объекта (свернуть / развернуть)</summary>

### 1. Заполнение транспортного справочника
  * `base_requests` - массив с описанием автобусных маршрутов и остановок;
  * `type` - строка, равная "Stop" или "Bus". Определяет тип передаваемого
    описания;
  * `name` - название остановки или маршрута;
  * `stops` - массив с названиями остановок, через которые проходит маршрут. У
    кольцевого маршрута название последней остановки дублирует название первой.
    Например: ["stop1", "stop2", "stop3", "stop1"];
  * `is_roundtrip` - значение типа bool. true, если маршрут кольцевой;
  * `latitude` - географическая широта остановки;
  * `longitude` - географическая долгота остановки;
  * `road_distances` - словарь, задающий дорожное расстояние от этой остановки
    до соседних. Каждый ключ в этом словаре — название соседней остановки,
    значение — целочисленное расстояние в метрах.

### 2. Настройки рендеринга карты
  * `render_settings` - словарь, задающий настройки визуализации карты;
  * `width` - ширина в пикселях;
  * `height` - высота в пикселях;
  * `padding` - отступ краёв карты от границ SVG-документа;
  * `stop_radius` - радиус окружностей, которыми обозначаются остановки;
  * `line_width` - толщина линий, которыми рисуются автобусные маршруты;
  * `bus_label_font_size` - размер текста, которым написаны названия автобусных
    маршрутов;
  * `bus_label_offset` - смещение надписи с названием маршрута относительно
    координат конечной остановки на карте;
  * `stop_label_font_size` - размер текста, которым отображаются названия
    остановок;
  * `stop_label_offset` - смещение названия остановки относительно её координат
    на карте;
  * `underlayer_color` - цвет подложки под названиями остановок и маршрутов;
  * `underlayer_width` - толщина подложки под названиями остановок и маршрутов;
  * `color_palette` - цветовая палитра.

### 3. Запросы к транспортному справочнику
  * `stat_requests` - массив с запросами к транспортному справочнику;
  * `id` - уникальный идентификатор запроса;
  * `type` - строка, равная "Stop", "Bus" или "Map". Определяет тип запроса;
  * `name` - название остановки или маршрута.
</details>

## Системные требования
1. С++17 (STL);
2. GCC 11.2 или Clang 13.

## Примеры входных и выходных данных

<details>
  <summary>Пример входящего JSON-объекта (свернуть / развернуть)</summary>

```json
{
  "base_requests": [
    {
      "type": "Bus",
      "name": "14",
      "stops": [
        "Улица Лизы Чайкиной",
        "Электросети",
        "Улица Докучаева",
        "Улица Лизы Чайкиной"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "114",
      "stops": [
        "Морской вокзал",
        "Ривьерский мост"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "name": "Ривьерский мост",
      "latitude": 43.587795,
      "longitude": 39.716901,
      "road_distances": {
        "Морской вокзал": 850
      }
    },
    {
      "type": "Stop",
      "name": "Морской вокзал",
      "latitude": 43.581969,
      "longitude": 39.719848,
      "road_distances": {
        "Ривьерский мост": 850
      }
    },
    {
      "type": "Stop",
      "name": "Электросети",
      "latitude": 43.598701,
      "longitude": 39.730623,
      "road_distances": {
        "Улица Докучаева": 3000,
        "Улица Лизы Чайкиной": 4300
      }
    },
    {
      "type": "Stop",
      "name": "Улица Докучаева",
      "latitude": 43.585586,
      "longitude": 39.733879,
      "road_distances": {
        "Улица Лизы Чайкиной": 2000,
        "Электросети": 3000
      }
    },
    {
      "type": "Stop",
      "name": "Улица Лизы Чайкиной",
      "latitude": 43.590317,
      "longitude": 39.746833,
      "road_distances": {
        "Электросети": 4300,
        "Улица Докучаева": 2000
      }
    }
  ],
  "render_settings": {
    "width": 600,
    "height": 400,
    "padding": 50,
    "stop_radius": 5,
    "line_width": 14,
    "bus_label_font_size": 20,
    "bus_label_offset": [
      7,
      15
    ],
    "stop_label_font_size": 20,
    "stop_label_offset": [
      7,
      -3
    ],
    "underlayer_color": [
      255,
      255,
      255,
      0.85
    ],
    "underlayer_width": 3,
    "color_palette": [
      "green",
      [
        255,
        160,
        0
      ],
      "red"
    ]
  },
  "stat_requests": [
    { "id": 1, "type": "Map" },
    { "id": 2, "type": "Stop", "name": "Ривьерский мост" },
    { "id": 3, "type": "Bus", "name": "114" }
  ]
}
```
</details>

<details>
  <summary>Ожидаемый вывод (свернуть / развернуть)</summary>

```json
[
  {
    "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n<polyline points=\"99.2283,329.5 50,232.18 99.2283,329.5\"  fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n<polyline points=\"550,190.051 279.22,50 333.61,269.08 550,190.051\"  fill=\"none\" stroke=\"rgb(255,160,0)\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n<text x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">114</text>\n<text x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\" fill=\"green\">114</text>\n<text x=\"50\" y=\"232.18\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">114</text>\n<text x=\"50\" y=\"232.18\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\" fill=\"green\">114</text>\n<text x=\"550\" y=\"190.051\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">14</text>\n<text x=\"550\" y=\"190.051\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\" fill=\"rgb(255,160,0)\">14</text>\n<circle cx=\"99.2283\" cy=\"329.5\" r=\"5\" fill=\"white\"/>\n<circle cx=\"50\" cy=\"232.18\" r=\"5\" fill=\"white\"/>\n<circle cx=\"333.61\" cy=\"269.08\" r=\"5\" fill=\"white\"/>\n<circle cx=\"550\" cy=\"190.051\" r=\"5\" fill=\"white\"/>\n<circle cx=\"279.22\" cy=\"50\" r=\"5\" fill=\"white\"/>\n<text x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">Морской вокзал</text>\n<text x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\">Морской вокзал</text>\n<text x=\"50\" y=\"232.18\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">Ривьерский мост</text>\n<text x=\"50\" y=\"232.18\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\">Ривьерский мост</text>\n<text x=\"333.61\" y=\"269.08\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">Улица Докучаева</text>\n<text x=\"333.61\" y=\"269.08\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\">Улица Докучаева</text>\n<text x=\"550\" y=\"190.051\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">Улица Лизы Чайкиной</text>\n<text x=\"550\" y=\"190.051\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\">Улица Лизы Чайкиной</text>\n<text x=\"279.22\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">Электросети</text>\n<text x=\"279.22\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\">Электросети</text>\n</svg>\n",
    "request_id": 1
  },
  {
    "buses": [
      "114"
    ],
    "request_id": 2
  },
  {
    "curvature": 1.23199,
    "request_id": 3,
    "route_length": 1700,
    "stop_count": 3,
    "unique_stop_count": 2
  }
]
```
</details>

<details>
  <summary>Пример сформированной карты маршрутов (свернуть / развернуть)</summary>

<img alt="map" width="1000" height="500" src="https://user-images.githubusercontent.com/62634576/166466541-e21036fe-ede9-4ccd-b35f-91eb43399f5c.svg">
</details>
