#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <stdexcept>
#include <fstream>
#include <libxml/parser.h>
#include <libxml/SAX2.h>
#include <libxml/xmlwriter.h>

struct Node
{
    Node(){}
    
    double lat, lon;
};

struct Way
{
    std::vector<long long> node_refs;
    bool is_highway = false;
    std::string name; // Road name if available
};


std::unordered_map<long long, Node> nodes;
std::vector<Way> ways;
std::unordered_map<long long, std::unordered_map<long long, std::string>> graph;

long long nodes_added =0;
long long nodes_total =0;

struct way_parser {
    // Parsing state
    struct ParseState
    {
        bool in_way = false;
        Way way;
    } state;

    static void startElement(void *ctx, const xmlChar *name, const xmlChar **attrs)
    {
        way_parser *parser = static_cast<way_parser *>(ctx);
        parser->start((const char *)name, attrs);
    }

    static void endElement(void *ctx, const xmlChar *name)
    {
        way_parser *parser = static_cast<way_parser *>(ctx);
        parser->end((const char *)name);
    }

    void start(const std::string &tagName, const xmlChar **attrs)
    {
        if (tagName == "way")
        {
            state.in_way = true;
            state.way = Way();
        }
        else if (state.in_way && tagName == "nd")
        {
            for (int i = 0; attrs && attrs[i]; i += 2)
            {
                std::string key = (const char *)attrs[i];
                std::string value = (const char *)attrs[i + 1];
                if (key == "ref")
                {
                    long long ref = std::stoll(value);
                    state.way.node_refs.push_back(ref);
                }
            }
        }
        else if (state.in_way && tagName == "tag")
        {
            std::string k, v;
            for (int i = 0; attrs && attrs[i]; i += 2)
            {
                std::string key = (const char *)attrs[i];
                std::string value = (const char *)attrs[i + 1];
                if (key == "k") k = value;
                else if (key == "v") v = value;
            }
            if (k == "highway")
                state.way.is_highway = true;
            else if (k == "name")
                state.way.name = v;
        }
    }

    void end(const std::string &tagName)
    {
        if (tagName == "way" && state.in_way)
        {
            if (state.way.is_highway)
            {
                for(auto id:state.way.node_refs){
                    nodes[id]=Node{};
                }
                ways.push_back(state.way);
                if (ways.size() % 10000 == 0)
                    std::cout << ways.size() / 1000 << "k ways parsed\n";
            }
            state.in_way = false;
        }
        
    }
};

struct node_parser {
    struct ParseState
    {
        bool in_node = false;
        Node current_node;
        long long current_id = 0;
    } state;

    static void startElement(void *ctx, const xmlChar *name, const xmlChar **attrs)
    {
        node_parser *parser = static_cast<node_parser *>(ctx);
        parser->start((const char *)name, attrs);
    }

    static void endElement(void *ctx, const xmlChar *name)
    {
        node_parser *parser = static_cast<node_parser *>(ctx);
        parser->end((const char *)name);
    }

    void start(const std::string &tagName, const xmlChar **attrs)
    {
        if (tagName == "node")
        {
            state.in_node = true;
            state.current_id = 0;
            state.current_node = Node();

            for (int i = 0; attrs && attrs[i]; i += 2)
            {
                std::string key = (const char *)attrs[i];
                std::string value = (const char *)attrs[i + 1];

                if (key == "id")
                    state.current_id = std::stoll(value);
                else if (key == "lon")
                    state.current_node.lon = std::stod(value);
                else if (key == "lat")
                    state.current_node.lat = std::stod(value);
            }
        }
    }

    void end(const std::string &tagName)
    {
        if (tagName == "node" && state.in_node)
        {
            nodes_total++;
            if (state.current_id != 0)
            {
                if (nodes.find(state.current_id)!=nodes.end()){
                nodes[state.current_id] = state.current_node;
                nodes_added++;
                if (nodes_added % 1000000 == 0)
                    std::cout << nodes_added / 1000000 << "M nodes parsed\n";
            }
            }
            state.in_node = false;
        }
    }
};

void parse_way_osm(const char *filename)
{
    way_parser parser;

    xmlSAXHandler handler = {};
    handler.startElement = way_parser::startElement;
    handler.endElement = way_parser::endElement;

    if (xmlSAXUserParseFile(&handler, &parser, filename) < 0)
    {
        throw std::runtime_error("Failed to parse OSM file");
    }

    xmlCleanupParser();
}
void parse_node_osm(const char *filename)
{
    node_parser parser;

    xmlSAXHandler handler = {};
    handler.startElement = node_parser::startElement;
    handler.endElement = node_parser::endElement;

    if (xmlSAXUserParseFile(&handler, &parser, filename) < 0)
    {
        throw std::runtime_error("Failed to parse OSM file");
    }

    xmlCleanupParser();
}


void write_osm_xml(const std::string &filename)
{
    std::ofstream out(filename);
    if (!out)
    {
        throw std::runtime_error("Failed to open output file");
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<osm version=\"0.6\" generator=\"CustomExtractor\">\n";

    // Write nodes
    for (const auto &pair : nodes)
    {
        long long id = pair.first;
        const Node &node = pair.second;
        out << "  <node id=\"" << id
            << "\" lat=\"" << node.lat
            << "\" lon=\"" << node.lon << "\" />\n";
    }

    // Write ways
    for (const auto &way : ways)
    {
        out << "  <way>\n";
        for (auto ref : way.node_refs)
        {
            out << "    <nd ref=\"" << ref << "\" />\n";
        }
        if (!way.name.empty())
        {
            out << "    <tag k=\"name\" v=\"" << way.name << "\" />\n";
        }
        out << "    <tag k=\"highway\" v=\"yes\" />\n";
        out << "  </way>\n";
    }

    out << "</osm>\n";
    out.close();
}

void write_sax_osm_xml(const std::string &filename)
{
    xmlTextWriterPtr writer = xmlNewTextWriterFilename(filename.c_str(), 0);
    if (writer == nullptr)
    {
        throw std::runtime_error("Error creating XML writer");
    }

    // Start the document
    xmlTextWriterStartDocument(writer, nullptr, "UTF-8", nullptr);
    xmlTextWriterStartElement(writer, BAD_CAST "osm");
    xmlTextWriterWriteAttribute(writer, BAD_CAST "version", BAD_CAST "0.6");
    xmlTextWriterWriteAttribute(writer, BAD_CAST "generator", BAD_CAST "CustomExtractor");

    // Write nodes
    for (const auto &pair : nodes)
    {
        long long id = pair.first;
        const Node &node = pair.second;

        xmlTextWriterStartElement(writer, BAD_CAST "node");
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id", "%lld", id);
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "lat", "%.8f", node.lat);
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "lon", "%.8f", node.lon);
        xmlTextWriterEndElement(writer); // </node>
    }

    // Write ways
    for (const auto &way : ways)
    {
        xmlTextWriterStartElement(writer, BAD_CAST "way");

        for (auto ref : way.node_refs)
        {
            xmlTextWriterStartElement(writer, BAD_CAST "nd");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ref", "%lld", ref);
            xmlTextWriterEndElement(writer); // </nd>
        }

        if (!way.name.empty())
        {
            xmlTextWriterStartElement(writer, BAD_CAST "tag");
            xmlTextWriterWriteAttribute(writer, BAD_CAST "k", BAD_CAST "name");
            xmlTextWriterWriteString(writer, BAD_CAST way.name.c_str());
            xmlTextWriterEndElement(writer); // </tag>
        }

        // Always write highway tag
        xmlTextWriterStartElement(writer, BAD_CAST "tag");
        xmlTextWriterWriteAttribute(writer, BAD_CAST "k", BAD_CAST "highway");
        xmlTextWriterWriteAttribute(writer, BAD_CAST "v", BAD_CAST "yes");
        xmlTextWriterEndElement(writer); // </tag>

        xmlTextWriterEndElement(writer); // </way>
    }

    // Close root element
    xmlTextWriterEndElement(writer); // </osm>

    // End the document
    xmlTextWriterEndDocument(writer);

    xmlFreeTextWriter(writer);
}


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input.osm> <output.osm> \n" ;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    try
    {
        parse_way_osm(input_file);
        std::cout << "ways:" << ways.size() / 1000.0 << "k ways parsed\n";
        std::cout << "nodes:" << nodes.size() / 1000000.0 << "m nodes to parse\n";
        
        parse_node_osm(input_file);
        std::cout << "total nodes:" << nodes_total / 1000000.0 << "m nodes to parse\n";
        
        write_osm_xml(output_file);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
