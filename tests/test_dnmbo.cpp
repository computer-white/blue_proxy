#include <gumbo.h>
#include <iostream>
#include <string>

void print_dom(GumboNode* node, int depth) {
    if (node->type != GUMBO_NODE_ELEMENT) return;
    
    // 打印缩进 + 标签名
    std::string indent(depth * 2, ' ');
    std::cout << indent << "<" << gumbo_normalized_tagname(node->v.element.tag);
    
    // 打印关键属性
    GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
    GumboAttribute* src  = gumbo_get_attribute(&node->v.element.attributes, "src");
    GumboAttribute* action = gumbo_get_attribute(&node->v.element.attributes, "action");
    if (href)   std::cout << " href=\"" << href->value << "\"";
    if (src)    std::cout << " src=\"" << src->value << "\"";
    if (action) std::cout << " action=\"" << action->value << "\"";
    
    std::cout << ">" << std::endl;
    
    // 递归处理子节点
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; i++) {
        print_dom((GumboNode*)children->data[i], depth + 1);
    }
}

int main()
{
    // 一个简单的 HTML 字符串
    std::string html = "<html><head><title>测试</title></head><body>"
                       "<a href=\"/s?wd=hello\">搜索</a>"
                       "<a href=\"http://www.baidu.com\">百度</a>"
                       "<img src=\"/img/logo.png\">"
                       "</body></html>";

    // 解析 HTML
    GumboOutput* output = gumbo_parse(html.c_str());
    print_dom(output->root,0);
    return 0;
}