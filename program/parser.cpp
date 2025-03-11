#ifndef PARSER_CPP
#define PARSER_CPP

#include "element.cpp"
#include <stack>
#include <fstream>
#include <sstream>
#include <set>
#include <regex>
#include <stdexcept>
#include <cctype>

class Parser
{
private:
    std::string rawText;
    size_t index;
    size_t len;
    std::vector<std::string> stack;
    std::shared_ptr<Element> root;

    void removeSpaces()
    {
        // 实现移除空格和换行符的逻辑
        while (index < len && (std::isspace(static_cast<unsigned char>(rawText[index])) || rawText[index] == '\n'))
        {
            ++index;
        }

        // 更新 rawText 和 len
        rawText = rawText.substr(index);
        len = rawText.length();
        index = 0;
    }

    void sliceText()
    {
        rawText = rawText.substr(index);
        len = rawText.length();
        index = 0;
    }

    std::string parseTag()
    {
        std::string tag;
        try
        {
            while (index < len && !isspace(rawText[index]) && rawText[index] != '>' && rawText[index] != '/')
            {
                tag += rawText[index++];
            }
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "parseTag错误：" << e.what() << std::endl;
            throw;
        }
        return tag;
    }

    std::string removeExtraSpaces(const std::string &input)
    {
        std::string result;
        bool spaceFlag = false;
        for (char character : input)
        {
            if (std::isspace(static_cast<unsigned char>(character)))
            {
                if (!spaceFlag)
                {
                    result += ' ';
                    spaceFlag = true;
                }
            }
            else
            {
                result += character;
                spaceFlag = false;
            }
        }
        return result;
    }

    void parseAttr(std::shared_ptr<Element> ele)
    {
        std::string a, b;
        while (index < len && rawText[index] != '=' && rawText[index] != '>')
        {
            a += rawText[index++];
        }

        sliceText();
        if (a.empty())
            return;

        ++index; // 跳过等号 '='
        char s = 0;
        if (rawText[index] == '\'' || rawText[index] == '\"')
        {
            s = rawText[index++];
        }

        while (index < len && rawText[index] != s)
        {
            b += rawText[index++];
        }

        ++index;
        ele->attributes.emplace_back(a, b);
        sliceText();
    }

    void parseAttrs(std::shared_ptr<Element> targetElement)
    {
        struct AttrInfo
        {
            std::string name;
            std::string content;
        };

        auto skipWhitespace = [&]()
        {
            const char whitespaces[] = " \t\n\r";
            while (index < len && strchr(whitespaces, rawText[index]))
                index++;
        };

        auto extractAttrName = [&]() -> AttrInfo
        {
            AttrInfo result;
            std::vector<char> nameBuffer;

            while (index < len)
            {
                char current = rawText[index];
                if (current == '=' || isspace(current) || current == '>')
                    break;
                nameBuffer.push_back(current);
                ++index;
            }

            result.name = std::string(nameBuffer.begin(), nameBuffer.end());
            return result;
        };

        auto parseAttrValue = [&](AttrInfo &attr) -> bool
        {
            if (index >= len || rawText[index] != '=')
                return false;

            ++index;
            skipWhitespace();

            if (index >= len)
                return false;

            if (rawText[index] == '\'' || rawText[index] == '\"')
            {
                const char delimiter = rawText[index++];
                std::stringstream valueStream;

                while (index < len && rawText[index] != delimiter)
                {
                    valueStream << rawText[index++];
                }

                if (index < len)
                    ++index;
                attr.content = valueStream.str();
                return true;
            }
            return false;
        };

        while (index < len && rawText[index] != '>')
        {
            skipWhitespace();
            if (index >= len || rawText[index] == '>')
                break;

            AttrInfo currentAttr = extractAttrName();
            if (parseAttrValue(currentAttr) && !currentAttr.name.empty())
            {
                targetElement->attributes.emplace_back(currentAttr.name, currentAttr.content);
            }
        }
    }
    const std::set<std::string> selfClosingTags =
        {"area", "base", "br", "col", "command", "embed", "hr", "img",
         "input", "keygen", "link", "meta", "param", "source", "track", "wbr",
         "basefont", "frame", "isindex"};
    // 判断是否是自闭合标签
    // 改进后的 isVoidElement 函数
    bool isVoidElement(const std::string &tag)
    {
        static const std::vector<std::string> voidElements = {
            "area", "base", "br", "col", "command", "embed", "hr", "img",
            "input", "keygen", "link", "meta", "param", "source", "track", "wbr",
            "basefont", "frame", "isindex"};

        // 移除前后空白
        std::string cleanTag = tag;
        cleanTag.erase(0, cleanTag.find_first_not_of(" \n\r\t"));
        cleanTag.erase(cleanTag.find_last_not_of(" \n\r\t") + 1);

        return std::find(voidElements.begin(), voidElements.end(), cleanTag) != voidElements.end();
    }

    void parseElement(std::shared_ptr<Element> parent)
    {
        try
        {
            if (!parent || index >= len)
            {
                return;
            }

            removeSpaces();
            if (index >= len)
                return;

            // 解析标签名并添加安全检查
            std::string tag;
            while (index < len && rawText[index] != ' ' && rawText[index] != '>' && rawText[index] != '/')
            {
                tag += rawText[index];
                ++index;
            }

            // 将标签名中的换行符替换为空格
            std::replace(tag.begin(), tag.end(), '\n', ' ');

            // 解析标签名并添加安全检查
            while (index < len && rawText[index] != ' ' && rawText[index] != '>' && rawText[index] != '/')
            {
                tag += rawText[index];
                ++index;
            }

            if (tag.empty())
            {
                throw std::runtime_error("空标签名");
            }

            auto ele = createElement(tag);
            ele->parent = parent;

            // 解析属性
            if (index < len && rawText[index] == ' ')
            {
                parseAttrs(ele);
            }

            // 跳过可能的空格
            removeSpaces();
            // 检查是否是自闭合标签
            bool isVoid = false;
            if (index < len)
            {
                // 获取标签名
                std::string tagName = ele->tagName;
                isVoid = isVoidElement(tagName.c_str());

                // 处理显式的自闭合符号 />
                if (rawText[index] == '/')
                {
                    ++index;
                    removeSpaces();

                    while (index < len && (rawText[index] == ' ' || rawText[index] == '\n'))
                    {
                        ++index;
                    }

                    if (index < len && rawText[index] == '>')
                    {
                        ++index;
                        parent->children.push_back(ele);
                        return;
                    }
                }
                // 如果是 void element,即使没有 /> 也认为是自闭合的
                else if (isVoid && rawText[index] == '>')
                {
                    ++index;
                    parent->children.push_back(ele);
                    return;
                }
            }
            // 检查普通标签结束
            if (index < len && rawText[index] == '>')
            {
                ++index;
            }
            else
            {
                throw std::runtime_error("标签格式错误");
            }

            // 如果是自闭合标签则直接返回
            if (selfClosingTags.find(tag) != selfClosingTags.end())
            {
                parent->children.push_back(ele);
                return;
            }

            // 处理普通标签
            parent->children.push_back(ele);
            stack.push_back(tag);

            // 解析子元素
            while (index < len)
            {
                removeSpaces();
                if (index >= len)
                    break;

                if (rawText[index] == '<')
                {
                    if (index + 1 >= len)
                        break;

                    ++index;
                    removeSpaces();

                    if (index < len && rawText[index] == '/')
                    {
                        ++index;
                        removeSpaces();

                        if (stack.empty())
                        {
                            throw std::runtime_error("未匹配的结束标签");
                        }

                        std::string startTag = stack.back();
                        std::string endTag;

                        // 解析结束标签
                        while (index < len && rawText[index] != '>' && !isspace(rawText[index]))
                        {
                            endTag += rawText[index];
                            ++index;
                        }
                        // 如果结束标签是P，将其转换为小写
                        if (endTag == "P")
                        {
                            std::transform(endTag.begin(), endTag.end(), endTag.begin(), ::tolower);
                        }

                        // 去除 startTag 和 endTag 中的空格
                        startTag.erase(std::remove_if(startTag.begin(), startTag.end(), ::isspace), startTag.end());
                        endTag.erase(std::remove_if(endTag.begin(), endTag.end(), ::isspace), endTag.end());

                        // 打印调试信息
                        std::cout << "startTag: " << startTag << ", endTag: " << endTag << std::endl;
                        if (startTag != endTag)
                        {
                            throw std::runtime_error("标签不匹配: 期望 </" + startTag + ">, 实际 </" + endTag + ">");
                        }

                        stack.pop_back();
                        // 跳过结束标签的 >
                        while (index < len && rawText[index] != '>')
                        {
                            ++index;
                        }
                        if (index < len)
                            ++index;
                        break;
                        if (index < len && rawText[index] == '>')
                        {
                            ++index;
                        }
                        else
                        {
                            throw std::runtime_error("标签格式错误");
                        }
                    }
                    else
                    {
                        parseElement(ele);
                    }
                }
                else
                {
                    parseText(ele);
                }
            }
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "解析元素越界错误：" << e.what() << std::endl;
            // throw;
        }
        catch (const std::exception &e)
        {
            std::cerr << "解析元素错误：" << e.what() << std::endl;
            // throw;
        }
    }

    class TextParser
    {
    private:
        struct TextAccumulator
        {
            std::string content;
            void append(char c) { content += c; }
            bool isEmpty() const { return content.empty(); }
            void clear() { content.clear(); }
            std::string get() const { return content; }
        };

        class TextValidator
        {
        public:
            static bool isTagStart(char curr, char next)
            {
                return curr == '<' && (std::isalnum(next) || next == '/');
            }

            static bool shouldContinue(const std::string &text, size_t pos, size_t len)
            {
                if (pos >= len)
                    return false;
                if (text[pos] != '<')
                    return true;
                return pos + 1 >= len || (!std::isalnum(text[pos + 1]) && text[pos + 1] != '/');
            }
        };

        class TextCleaner
        {
        public:
            static std::string process(const std::string &input)
            {
                std::string result;
                bool prevSpace = true;

                for (char c : input)
                {
                    if (std::isspace(c))
                    {
                        if (!prevSpace)
                        {
                            result += ' ';
                            prevSpace = true;
                        }
                    }
                    else
                    {
                        result += c;
                        prevSpace = false;
                    }
                }

                if (!result.empty() && result.back() == ' ')
                {
                    result.pop_back();
                }

                return result;
            }
        };

    public:
        static void parse(const std::string &rawText,
                          size_t &index,
                          size_t len,
                          std::shared_ptr<Element> parent)
        {
            if (!parent || index >= len)
            {
                return;
            }

            try
            {
                TextAccumulator accumulator;

                while (TextValidator::shouldContinue(rawText, index, len))
                {
                    accumulator.append(rawText[index++]);
                }

                if (!accumulator.isEmpty())
                {
                    std::string cleanedText = TextCleaner::process(accumulator.get());
                    if (!cleanedText.empty())
                    {
                        parent->children.push_back(createTextElement(cleanedText));
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "文本解析错误: " << e.what() << std::endl;
                throw;
            }
        }
    };

    void parseText(std::shared_ptr<Element> parent)
    {
        TextParser::parse(rawText, index, len, parent);
    }

public:
    void parse(char *text, std::shared_ptr<Element> rootNode)
    {
        if (!text || !rootNode)
        {
            return;
        }

        try
        {
            rawText = std::string(text);
            // 移除前后空格
            if (!rawText.empty())
            {
                size_t start = rawText.find_first_not_of(" \n\r\t\f\v");
                if (start != std::string::npos)
                {
                    size_t end = rawText.find_last_not_of(" \n\r\t\f\v");
                    rawText = rawText.substr(start, end - start + 1);
                }
            }

            len = rawText.length();
            index = 0;
            stack.clear();

            while (index < len)
            {
                removeSpaces();
                if (index < len)
                { // 添加边界检查
                    if (rawText[index] == '<')
                    {
                        index++;
                        if (index < len)
                        { // 添加边界检查
                            parseElement(rootNode);
                        }
                    }
                    else
                    {
                        parseText(rootNode);
                    }
                }
            }
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "解析错误：字符串访问越界 - " << e.what() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "解析错误：" << e.what() << std::endl;
        }
    }
    void printParsedTree(std::shared_ptr<Element> root) const
    {
        if (root)
        {
            root->print(0);
        }
    }
};

#endif