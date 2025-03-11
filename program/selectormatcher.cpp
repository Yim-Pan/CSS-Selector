#ifndef SELECOTRMATCHER_CPP
#define SELECOTRMATCHER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <stdexcept>
#include <cctype>
#include "element.cpp"
#include "parser.cpp"

bool isChineseCharacter(char ch)
{
    // 检查字符是否是中文字符
    return (ch & 0x80) != 0;
}

// 将CSS选择器字符串分解为标记tokens，处理特殊字符：'>', '+', '~', ',', ' '用于后续的选择器匹配
// text - 输入的CSS选择器字符串，返回包含所有标记的字符串向量。
std::vector<std::string> tokenize(const std::string &text)
{
    std::vector<std::string> tokens;
    std::string current;
    // 遍历选择器字符串的每个字符
    for (char ch : text)
    {
        // 检查是否是特殊字符（选择器组合符）
        if (ch == '>' || ch == '+' || ch == '~' || ch == ',' || ch == ' ')
        {
            // 如果当前标记不为空，先保存它
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear(); // 清空当前标记以准备下一个
            }
            tokens.push_back(std::string(1, ch));
        }
        else
        {
            current += ch;
        }
    }
    // 处理最后一个标记（如果存在）
    if (!current.empty())
    {
        tokens.push_back(current);
    }
    return tokens;
}

bool MatchSelector(const std::shared_ptr<Element> &element, const std::string &selector)
{
    // 实现五个选择器，分别是:root,:empty,::first-letter,:lang(language),:not(),
    if (selector == "*")
    {
        return true;
    }
    else if (selector == ":root")
    {
        // 打印 parent 是否为空，并输出该元素的 tagName
        return element->tagName == "html";
    }

    // 处理:empty伪类
    size_t colonPos = selector.find(':');
    if (colonPos != std::string::npos)
    {
        std::string baseSelector = selector.substr(0, colonPos);
        std::string pseudoClass = selector.substr(colonPos + 1);

        // 处理双冒号伪元素选择器
        if (pseudoClass.find(':') == 0)
        {
            pseudoClass = pseudoClass.substr(1);
        }

        if (pseudoClass == "empty")
        {
            if (!baseSelector.empty() && baseSelector != element->tagName)
            {
                return false;
            }
            return element->children.empty();
        }
        else if (pseudoClass.find("lang(") == 0)
        {
            size_t openParenPos = pseudoClass.find('(');
            size_t closeParenPos = pseudoClass.find(')');
            if (openParenPos != std::string::npos && closeParenPos != std::string::npos)
            {
                std::string langValue = pseudoClass.substr(openParenPos + 1, closeParenPos - openParenPos - 1);
                if (!baseSelector.empty() && baseSelector != element->tagName)
                {
                    return false;
                }
                bool langFound = false;
                for (const auto &[key, value] : element->attributes)
                {
                    if (key == "lang" && value == langValue)
                    {
                        langFound = true;
                        break;
                    }
                }
                return langFound;
            }
        }
        else if (pseudoClass == "first-letter")
        {
            if (!baseSelector.empty() && baseSelector != element->tagName)
            {
                return false;
            }
            // 检查第一个子节点是否是文本节点，并且文本节点的第一个字符是否存在
            if (!element->children.empty())
            {
                auto firstChild = std::dynamic_pointer_cast<Text>(element->children.front());
                if (firstChild && !firstChild->nodeValue.empty())
                {
                    // 打印第一个字符，无论是英文还是中文
                    std::cout << "First letter: " << firstChild->nodeValue[0];
                    if (isChineseCharacter(firstChild->nodeValue[0]))
                    {
                        std::cout << firstChild->nodeValue[1] << firstChild->nodeValue[2];
                    }
                    std::cout << std::endl;
                    return true;
                }
            }
            return false;
        }
        else if (pseudoClass.find("not(") == 0)
        {
            size_t openParenPos = pseudoClass.find('(');
            size_t closeParenPos = pseudoClass.find(')');
            if (openParenPos != std::string::npos && closeParenPos != std::string::npos)
            {
                std::string notSelector = pseudoClass.substr(openParenPos + 1, closeParenPos - openParenPos - 1);
                return !MatchSelector(element, notSelector);
            }
        }
    }

    std::string NormSelector;
    // 标准化
    for (char ch : selector)
    {
        if (ch == '.')
        {
            NormSelector += " ."; // .前添加空格
        }
        else if (ch == '#')
        {
            NormSelector += " #"; // #前添加空格
        }
        else
        {
            NormSelector += ch;
        }
    }

    // 分割并将子选择器存入vector
    std::istringstream iss(NormSelector);
    std::vector<std::string> subSelectors;
    std::string subSelector;
    while (std::getline(iss, subSelector, ' '))
    {
        if (!subSelector.empty())
        {
            subSelectors.push_back(subSelector);
        }
    }

    // 没有子选择器返回 false
    if (subSelectors.empty())
    {
        return false;
    }

    // 检查标签、类和 ID。如果任何一个选择器不匹配，返回false；全部匹配则返回true。
    bool isTagMatched = false, isClassOrIdMatched = true, isMatched = false;

    for (const auto &sel : subSelectors)
    {
        if (sel.empty())
        {
            continue;
        }
        if (sel[0] != '#' && sel[0] != '.')
        {
            // 检查标签
            if (sel != element->tagName)
            {
                return false;
            }
        }
        else if (sel[0] == '#')
        {
            // 检查 ID
            bool idFound = false;
            for (const auto &[key, value] : element->attributes)
            {
                if (key == "id" && value == sel.substr(1))
                {
                    idFound = true;
                    break;
                }
            }
            if (!idFound)
            {
                return false;
            }
        }
        else if (sel[0] == '.')
        {

            // 检查每个类
            bool classFound = false;
            for (const auto &[key, value] : element->attributes)
            {
                if (key == "class")
                {
                    std::string classAttribute = value;
                    std::string className = sel.substr(1);
                    if (classAttribute.find(className) != std::string::npos)
                    {
                        classFound = true;
                        break;
                    }
                }
            }
            if (!classFound)
            {
                return false;
            }
        }
    }
    return true;
}

void MatchElementfind(const std::shared_ptr<Element> &root, const std::string &selector, std::vector<std::shared_ptr<Element>> &matchedElements)
{
    // 检查当前元素是否匹配选择器
    if (MatchSelector(root, selector))
    {
        matchedElements.push_back(root);
    }

    // 递归遍历子元素
    for (const auto &child : root->children)
    {
        if (child->nodeType == NodeType::Element)
        {
            MatchElementfind(std::dynamic_pointer_cast<Element>(child), selector, matchedElements);
        }
    }
}
// 选择器类型枚举
enum class SelectorType
{
    Simple,     // 单个选择器
    Child,      // > 子选择器
    Descendant, // 空格 后代选择器
    Adjacent,   // + 相邻兄弟
    General,    // ~ 通用兄弟
    Multiple    // , 多选择器
};

// CSS选择器处理类
class CssSelectorMatcher
{
private:
    // 首先定义ElementSet类型（两个语句都必不可少）
    using ElementSet = std::set<std::shared_ptr<Element>>;
    std::shared_ptr<Element> root;

    // 定义辅助函数processSelector
    std::vector<std::shared_ptr<Element>> processSelector(const std::vector<std::string> &selectorParts,
                                                          const std::shared_ptr<Element> &root)
    {
        ElementSet results;

        if (selectorParts.empty())
        {
            return std::vector<std::shared_ptr<Element>>();
        }

        if (selectorParts.size() == 1)
        {
            auto elements = findElements(selectorParts[0]);
            for (const auto &element : elements)
            {
                results.insert(element);
            }
        }
        else
        {
            auto firstMatched = findElements(selectorParts[0]);
            for (const auto &element : firstMatched)
            {
                auto nextResults = matchRecursive(selectorParts, 1, element);
                results.insert(nextResults.begin(), nextResults.end());
            }
        }

        return std::vector<std::shared_ptr<Element>>(results.begin(), results.end());
    }
    // 处理+
    std::vector<std::shared_ptr<Element>> AdjacentSelect(const std::string &precSelector, const std::string &adjacentSelector, const std::shared_ptr<Element> &root)
    {
        std::vector<std::shared_ptr<Element>> matchPrecElements;
        std::set<std::shared_ptr<Element>> uniqueMatch;

        MatchElementfind(root, precSelector, matchPrecElements);

        for (const auto &precElement : matchPrecElements)
        {
            auto parent = precElement->parent.lock();
            if (parent)
            {
                bool flag = false;
                for (const auto &child : parent->children)
                {
                    if (flag)
                    {
                        auto siblingElement = std::dynamic_pointer_cast<Element>(child);
                        if (siblingElement && MatchSelector(siblingElement, adjacentSelector))
                        {
                            uniqueMatch.insert(siblingElement);
                            break; // 只考虑第一个紧邻的兄弟元素
                        }
                    }
                    if (child == precElement)
                    {
                        flag = true;
                    }
                }
            }
        }

        // 转换为 vector 输出
        std::vector<std::shared_ptr<Element>> finalMatch(uniqueMatch.begin(), uniqueMatch.end());
        return finalMatch;
    }

    // 处理～
    std::vector<std::shared_ptr<Element>> GenSelect(const std::string &precedSelector, const std::string &siblingSelector, const std::shared_ptr<Element> &root)
    {
        std::vector<std::shared_ptr<Element>> matchedPrecedingElements;
        MatchElementfind(root, precedSelector, matchedPrecedingElements);

        std::set<std::shared_ptr<Element>> matchedSet;
        for (const auto &precedingElement : matchedPrecedingElements)
        {
            auto parent = std::dynamic_pointer_cast<Element>(precedingElement->parent.lock());
            if (parent)
            {
                bool startMatching = false;
                for (const auto &child : parent->children)
                {
                    if (child == precedingElement)
                    {
                        startMatching = true;
                        continue;
                    }
                    if (startMatching)
                    {
                        auto childElement = std::dynamic_pointer_cast<Element>(child);
                        if (childElement && MatchSelector(childElement, siblingSelector))
                        {
                            matchedSet.insert(childElement);
                        }
                    }
                }
            }
        }

        // 转换为 vector 输出
        std::vector<std::shared_ptr<Element>> finalMatchedElements(matchedSet.begin(), matchedSet.end());
        return finalMatchedElements;
    }
    // MultipleSelect函数
    std::vector<std::shared_ptr<Element>> MultipleSelect(const std::vector<std::string> &parts,
                                                         const std::shared_ptr<Element> &root)
    {
        ElementSet results;
        std::vector<std::string> currentSelector;

        for (size_t i = 0; i < parts.size(); i++)
        {
            if (parts[i] == ",")
            {
                auto currentResults = processSelector(currentSelector, root);
                results.insert(currentResults.begin(), currentResults.end());
                currentSelector.clear();
            }
            else
            {
                currentSelector.push_back(parts[i]);
            }
        }

        if (!currentSelector.empty())
        {
            auto currentResults = processSelector(currentSelector, root);
            results.insert(currentResults.begin(), currentResults.end());
        }

        return std::vector<std::shared_ptr<Element>>(results.begin(), results.end());
    }

    // 获取选择器类型
    SelectorType getSelectorType(const std::string &combinator)
    {
        if (combinator == ">")
            return SelectorType::Child;
        if (combinator == "+")
            return SelectorType::Adjacent;
        if (combinator == "~")
            return SelectorType::General;
        if (combinator == ",")
            return SelectorType::Multiple;
        if (combinator == " ")
            return SelectorType::Descendant;
        return SelectorType::Simple;
    }

    // 基础元素查找
    std::vector<std::shared_ptr<Element>> findElements(const std::string &selector)
    {
        std::vector<std::shared_ptr<Element>> results;
        MatchElementfind(root, selector, results);
        return results;
    }

    // 子元素匹配
    ElementSet matchChildren(const std::shared_ptr<Element> &parent, const std::string &selector)
    {
        ElementSet results;
        for (const auto &child : parent->children)
        {
            if (auto element = std::dynamic_pointer_cast<Element>(child))
            {
                if (MatchSelector(element, selector))
                {
                    results.insert(element);
                }
            }
        }
        return results;
    }

    // 后代元素匹配
    ElementSet matchDescendants(const std::shared_ptr<Element> &parent, const std::string &selector)
    {
        ElementSet results;
        for (const auto &child : parent->children)
        {
            if (auto element = std::dynamic_pointer_cast<Element>(child))
            {
                if (MatchSelector(element, selector))
                {
                    results.insert(element);
                }
                auto childResults = matchDescendants(element, selector);
                results.insert(childResults.begin(), childResults.end());
            }
        }
        return results;
    }

    // 递归匹配选择器部分
    ElementSet matchRecursive(const std::vector<std::string> &parts, size_t index, const std::shared_ptr<Element> &currentElement)
    {
        if (index >= parts.size())
        {
            ElementSet result;
            result.insert(currentElement);
            return result;
        }
        ElementSet results;
        auto selectorType = getSelectorType(parts[index]);

        switch (selectorType)
        {
        case SelectorType::Simple:
        {
            if (MatchSelector(currentElement, parts[index]))
            {
                results.insert(currentElement);
            }
            break;
        }
        case SelectorType::Child:
        {
            auto children = matchChildren(currentElement, parts[index + 1]);
            for (const auto &child : children)
            {
                auto childResults = matchRecursive(parts, index + 2, child);
                results.insert(childResults.begin(), childResults.end());
            }
            break;
        }
        case SelectorType::Descendant:
        {
            auto descendants = matchDescendants(currentElement, parts[index + 1]);
            for (const auto &descendant : descendants)
            {
                auto descendantResults = matchRecursive(parts, index + 2, descendant);
                results.insert(descendantResults.begin(), descendantResults.end());
            }
            break;
        }
        default:
        {
            throw std::runtime_error("Unknown selector type");
        }
        }
        return results;
    }

public:
    CssSelectorMatcher(const std::shared_ptr<Element> &rootElement) : root(rootElement) {}
    std::vector<std::shared_ptr<Element>> match(const std::string &selector)
    {
        auto parts = tokenize(selector);
        ElementSet results;
        // 检查是否包含逗号
        bool hasComma = std::find(parts.begin(), parts.end(), ",") != parts.end();
        if (hasComma)
        {
            // 使用新的MultipleSelect处理逗号分隔的选择器
            return MultipleSelect(parts, root);
        }
        else
        {
            if (parts.size() == 1)
            {
                // 单个选择器的情况
                auto elements = findElements(parts[0]);
                results.insert(elements.begin(), elements.end());
            }
            else
            {
                // 处理多层级选择器
                auto firstMatched = findElements(parts[0]);

                // 如果是两个以上的部分
                if (parts.size() >= 3)
                {
                    auto selectorType = getSelectorType(parts[1]);

                    switch (selectorType)
                    {
                    case SelectorType::Simple:
                    {
                        results.insert(firstMatched.begin(), firstMatched.end());
                        break;
                    }
                    case SelectorType::Child:
                    {
                        for (const auto &parent : firstMatched)
                        {
                            auto children = matchChildren(parent, parts[2]);
                            results.insert(children.begin(), children.end());
                        }
                        // 处理剩余的部分
                        if (parts.size() > 3)
                        {
                            ElementSet tempResults;
                            for (const auto &element : results)
                            {
                                auto nextResults = matchRecursive(parts, 3, element);
                                tempResults.insert(nextResults.begin(), nextResults.end());
                            }
                            results = tempResults;
                        }
                        break;
                    }
                    case SelectorType::Descendant:
                    {
                        for (const auto &ancestor : firstMatched)
                        {
                            auto descendants = matchDescendants(ancestor, parts[2]);
                            results.insert(descendants.begin(), descendants.end());
                        }
                        // 处理剩余的部分
                        if (parts.size() > 3)
                        {
                            ElementSet tempResults;
                            for (const auto &element : results)
                            {
                                auto nextResults = matchRecursive(parts, 3, element);
                                tempResults.insert(nextResults.begin(), nextResults.end());
                            }
                            results = tempResults;
                        }
                        break;
                    }
                    case SelectorType::Adjacent:
                    {
                        auto adjacent = AdjacentSelect(parts[0], parts[2], root);
                        results.insert(adjacent.begin(), adjacent.end());
                        break;
                    }
                    case SelectorType::General:
                    {
                        auto siblings = GenSelect(parts[0], parts[2], root);
                        results.insert(siblings.begin(), siblings.end());
                        break;
                    }
                    case SelectorType::Multiple:
                    {
                        std::vector<std::string> selectorParts;
                        // 使用与函数声明匹配的size_t类型
                        const size_t startPos = 0; // 从0开始收集

                        // 收集第一个选择器的部分
                        while (startPos < parts.size() && parts[startPos] != ",")
                        {
                            selectorParts.push_back(parts[startPos]);
                        }

                        // 找到逗号的位置
                        size_t commaPos = startPos;
                        while (commaPos < parts.size() && parts[commaPos] != ",")
                        {
                            ++commaPos;
                        }

                        // 从逗号后开始收集第二个选择器的部分
                        if (commaPos < parts.size())
                        {
                            ++commaPos; // 跳过逗号
                            while (commaPos < parts.size())
                            {
                                selectorParts.push_back(parts[commaPos]);
                                ++commaPos;
                            }
                        }

                        auto multiple = MultipleSelect(selectorParts, root);
                        for (const auto &elem : multiple)
                        {
                            results.insert(elem);
                        }
                        break;
                    }
                    }
                }
                else
                {
                    // 处理两部分的选择器
                    for (const auto &element : firstMatched)
                    {
                        auto elementResults = matchRecursive(parts, 1, element);
                        results.insert(elementResults.begin(), elementResults.end());
                    }
                }
            }
        }
        return std::vector<std::shared_ptr<Element>>(results.begin(), results.end());
    }
};

#endif