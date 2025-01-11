#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include "TokenManager.h"
#include "UserManager.h"
#include "TestManager.h"
#include <nlohmann/json.hpp>  // Подключаем библиотеку для работы с JSON

// Простая структура для имитации взаимодействия с пользователями
std::map<std::string, std::string> sessionTokens;

// Прототип функции
void parseAndDisplayTests(const std::string& jsonResponse);

size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userData) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userData);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void displayMenu() {
    std::cout << "\n=== Главный Модуль ===\n";
    std::cout << "1. Авторизация\n";
    std::cout << "2. Просмотр профиля пользователя\n";
    std::cout << "3. Создание теста\n";
    std::cout << "4. Завершить\n";
    std::cout << "Выберите действие: ";
}

void sendPostRequest(const std::string& url, const std::string& data) {
    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Ошибка выполнения запроса: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Запрос успешно выполнен.\n";
        }

        curl_easy_cleanup(curl);
    }
}

void handleAuthorization() {
    std::string email;
    std::cout << "Введите email: ";
    std::cin >> email;

    std::string token = TokenManager::createToken(email, {"student"});
    sessionTokens[email] = token;

    std::cout << "Авторизация успешна. Токен: " << token << "\n";
}

void viewUserProfile() {
    std::string email;
    std::cout << "Введите email для проверки: ";
    std::cin >> email;

    if (sessionTokens.find(email) != sessionTokens.end()) {
        std::cout << "Пользователь: " << email << "\n";
        std::cout << "Токен: " << sessionTokens[email] << "\n";
    } else {
        std::cout << "Пользователь не авторизован.\n";
    }
}

void createTest() {
    std::string testName;
    int questionCount;
    std::cout << "Введите название теста: ";
    std::cin.ignore();
    std::getline(std::cin, testName);

    std::cout << "Введите количество вопросов: ";
    std::cin >> questionCount;

    std::vector<std::string> questions;
    std::cin.ignore();
    for (int i = 0; i < questionCount; ++i) {
        std::string question;
        std::cout << "Вопрос " << i + 1 << ": ";
        std::getline(std::cin, question);
        questions.push_back(question);
    }

    // Формирование JSON-данных
    std::string data = R"({"name":")" + testName + R"(","questions":[)";
    for (size_t i = 0; i < questions.size(); ++i) {
        data += "\"" + questions[i] + "\"";
        if (i < questions.size() - 1) data += ",";
    }
    data += "]}";
    
    // Отправка данных на сервер
    std::string url = "http://localhost:8080/api/tests";
    sendPostRequest(url, data);

    std::cout << "Тест \"" << testName << "\" успешно отправлен.\n";
}

void viewTests() {
    CURL* curl = curl_easy_init();
    std::string response;  // Переменная для хранения ответа сервера

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/api/tests");

        // Получение ответа от сервера
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Ошибка выполнения запроса: " << curl_easy_strerror(res) << std::endl;
        } else {
            // Обработка и вывод результата
            parseAndDisplayTests(response);
        }

        curl_easy_cleanup(curl);
    }
}

void parseAndDisplayTests(const std::string& jsonResponse) {
    try {
        nlohmann::json tests = nlohmann::json::parse(jsonResponse);

        std::cout << "\n=== Список тестов ===\n";
        for (const auto& test : tests) {
            std::cout << "Тест: " << test["testName"] << "\n";
            std::cout << "Вопросы:\n";
            for (const auto& question : test["questions"]) {
                std::cout << "  - " << question << "\n";
            }
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга JSON: " << e.what() << std::endl;
    }
}

int main() {
    int choice = 0;
    do {
        displayMenu();
        std::cin >> choice;

        switch (choice) {
            case 1:
                handleAuthorization();
                break;
            case 2:
                viewUserProfile();
                break;
            case 3:
                createTest();
                break;
            case 4:
                std::cout << "Завершение работы.\n";
                break;
            default:
                std::cout << "Неверный выбор. Попробуйте снова.\n";
        }
    } while (choice != 4);

    return 0;
}
