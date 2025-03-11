#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <stack>
#include <fstream>
#include <sstream>
#include <set>
#include <regex>
#include <stdexcept>
#include <cctype>
#include "element.cpp"
#include "parser.cpp"
#include "selectormatcher.cpp"

char *readFile(const std::string &filePath)
{
    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
    if (!fileStream.is_open())
    {
        std::cerr << "无法打开文件: " << std::endl;
        return nullptr;
    }

    std::streamsize size = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    char *buffer = new char[size + 1];
    if (fileStream.read(buffer, size))
    {
        buffer[size] = '\0'; // 添加结束符
        return buffer;
    }
    else
    {
        delete[] buffer;
        return nullptr;
    }
}

void removeBlock(char *content, const char *startTag, const char *endTag)
{
    char *star = strstr(content, startTag);
    while (star)
    {
        char *end = strstr(star, endTag);
        if (!end)
        {
            break;
        }

        end += strlen(endTag);
        memmove(star, end, strlen(end) + 1); // 删除

        star = strstr(star, startTag); // 寻找下一个开始标签
    }
}

// 简化html
void simplify(char *c)
{
    removeBlock(c, "<!--", "-->"); // 删注释
    removeBlock(c, "<!DOCTYPE", "html>");
    removeBlock(c, "<!doctype", "html>");
    removeBlock(c, "<script", "</script>");
    removeBlock(c, "<style", "</style>");
}

void InnerText(const std::shared_ptr<Node> &node)
{
    if (node->nodeType == NodeType::Text)
    {
        // 转换到 Text 类型以访问 nodeValue
        auto textNode = std::dynamic_pointer_cast<Text>(node);
        std::cout << textNode->nodeValue << std::endl;
    }
    else if (node->nodeType == NodeType::Element)
    {
        // 转换到 Element 类型以访问子节点
        auto element = std::dynamic_pointer_cast<Element>(node);
        for (const auto &child : element->children)
        {
            InnerText(child); // 递归调用打印子节点的文本
        }
    }
}

void OuterHtml(const std::shared_ptr<Element> &element)
{
    element->print(0);
}

// 使用
std::vector<std::shared_ptr<Element>> useSelector(const std::string &selector, const std::shared_ptr<Element> &root)
{
    CssSelectorMatcher matcher(root);
    return matcher.match(selector);
}

void Hrefs(const std::shared_ptr<Element> &element)
{
    if (!element)
    {
        return;
    }

    // 检查当前元素是否为 a 标签
    if (element->tagName == "a")
    {
        for (const auto &[key, value] : element->attributes)
        {
            if (key == "href")
            {
                std::cout << "找到href: " << value << std::endl;
            }
        }
    }

    // 递归处理子元素
    for (const auto &child : element->children)
    {
        if (auto childElement = std::dynamic_pointer_cast<Element>(child))
        {
            Hrefs(childElement);
        }
    }
}

// 添加函数用于使用wget下载
bool downloadWithWget(const std::string &url)
{
    // 生成随机文件名
    std::string tempFile = "downloaded_" + std::to_string(time(nullptr)) + ".html";

    // 构建wget命令
    std::string command = "wget -q -O " + tempFile + " " + url;
    int result = system(command.c_str());
    return (result == 0) ? true : false;
}
void run();

void Selection(const std::shared_ptr<Element> &root)
{
    std::string cssSelector;
    std::cout << "please input cssSelector: " << std::endl;
    std::getline(std::cin, cssSelector);

    auto matchedElements = useSelector(cssSelector, root);
    int num = 0;
    std::cout << "matched elements:" << std::endl;
    for (const auto &elem : matchedElements)
    {
        if (elem)
        {
            if (elem->tagName == "root")
            {
                continue;
            }
            num++;
            std::cout << elem->tagName;
            // 检查并打印 class 和 id 属性
            for (const auto &[key, value] : elem->attributes)
            {
                if (key == "class")
                {
                    std::istringstream classStream(value);
                    std::string className;
                    while (std::getline(classStream, className, ' '))
                    {
                        std::cout << "." << className;
                    }
                }
                else if (key == "id")
                {
                    std::cout << "#" << value;
                }
            }
            std::cout << std::endl;
        }
    }
    std::cout << "NodeList contains " << num << " elements" << std::endl;
    std::cout << "choose operation:\n1. innerText\n2. outerHTML\n3. href\n4. query\n5. quit\n6. change path\n7. change cssSelector\n";
    int option;
    std::cin >> option;
    switch (option)
    {
    case 1:
        std::cout << "choose node index (from 0): ";
        size_t nodeIndex1;
        std::cin >> nodeIndex1;

        if (nodeIndex1 < matchedElements.size())
        {
            auto selected = matchedElements[nodeIndex1];
            InnerText(selected);
        }
        else
        {
            std::cout << "Invalid node index." << std::endl;
        }
        std::cin.ignore(); // 忽略之前的换行符
        Selection(root);   // 重新调用 handleUserSelection 以更换 CSS 选择器
        return;
    case 2:
        std::cout << "choose node index (from 0): ";
        size_t nodeIndex2;
        std::cin >> nodeIndex2;

        if (nodeIndex2 < matchedElements.size())
        {
            auto selected = matchedElements[nodeIndex2];
            OuterHtml(selected);
        }
        else
        {
            std::cout << "Invalid node index." << std::endl;
        }
        std::cin.ignore(); // 忽略之前的换行符
        Selection(root);   // 重新调用 handleUserSelection 以更换 CSS 选择器
        return;
    case 3:
        std::cout << "choose node index (from 0): ";
        size_t nodeIndex3;
        std::cin >> nodeIndex3;

        if (nodeIndex3 < matchedElements.size())
        {
            auto selected = matchedElements[nodeIndex3];
            if (selected)
            {
                std::cout << "Searching for hrefs..." << std::endl;
                Hrefs(selected);
            }
        }
        else
        {
            std::cout << "Invalid node index." << std::endl;
        }
        std::cin.ignore(); // 忽略之前的换行符
        Selection(root);   // 重新调用 handleUserSelection 以更换 CSS 选择器
        return;
    case 4:
        std::cout << "choose node index (from 0): ";
        size_t nodeIndex4;
        std::cin >> nodeIndex4;

        if (nodeIndex4 < matchedElements.size())
        {
            auto selectedElement = matchedElements[nodeIndex4];
            std::cin.ignore(); // 忽略之前的换行符
            Selection(selectedElement);
        }
        else
        {
            std::cout << "Invalid node index." << std::endl;
        }
        break;
    case 5:
        std::cin.ignore(); // 忽略之前的换行符
        break;
    case 6:
        std::cin.ignore(); // 忽略之前的换行符
        run();             // 调用 run 函数以更换 HTML 路径
        return;
    case 7:
        std::cin.ignore(); // 忽略之前的换行符
        Selection(root);   // 重新调用 handleUserSelection 以更换 CSS 选择器
        return;
    default:
        std::cout << "Invalid option." << std::endl;
        std::cin.ignore(); // 忽略之前的换行符
        Selection(root);   // 重新调用 handleUserSelection 以更换 CSS 选择器
        return;
    }
}

int main()
{
    run();
    return 0;
}

void run()
{
    std::string input;
    std::cout << "请输入HTML文件路径或URL (以http://或https://开头): ";
    std::getline(std::cin, input);
    char *html = nullptr;
    std::string tempFile;
    // 添加输入长度检查
    if (input.length() >= 7 && // 确保字符串长度足够检查前缀
        (input.substr(0, 7) == "http://" ||
         (input.length() >= 8 && input.substr(0, 8) == "https://")))
    {

        // 生成临时文件名
        tempFile = "downloaded_" + std::to_string(time(nullptr)) + ".html";

        // 使用wget下载
        if (!downloadWithWget(input))
        {
            std::cerr << "无法下载网页内容" << std::endl;
            return;
        }

        // 读取下载的文件
        html = readFile(tempFile);
    }
    else
    {
        // 处理本地文件
        html = readFile(input);
    }

    if (html != nullptr)
    {
        simplify(html);
        Parser parser;
        auto rootNode = createElement("root");
        parser.parse(html, rootNode);
        Selection(rootNode);
        delete[] html;
    }
    else
    {
        std::cerr << "无法读取内容" << std::endl;
    }
}