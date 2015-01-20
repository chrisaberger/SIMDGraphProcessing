// class templates
#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include <getopt.h>

using namespace pcm_helper;

class Parser{
  public:
    int num_threads;
    bool attributes;
    int n;
    long start_node;
    MutableGraph *input_graph;
    string layout;

    Parser(int num_threads_in, bool attributes_in,
      int n_in, size_t start_node_in, MutableGraph *in_graph, string layout_in){
      num_threads = num_threads_in;
      attributes = attributes_in;
      n = n_in;
      start_node = start_node_in;
      input_graph = in_graph;
      layout = layout_in;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
//Main setup code
//////////////////////////////////////////////////////////////////////////////////////////
//Ideally the user shouldn't have to concern themselves with what happens down here.
namespace input_parser{
  inline void printUsage(string app){
    cout << "USAGE: ./application <OPTIONS>" << endl;
    cout << "OPTIONS: " << endl;
    cout <<"\tREQUIRED: --graph=<path to graph> --input_type=<\'binary\' or \'text\'> --t=<# of threads>" << endl;
    cout << "\t\t--layout=<uint,pshort,bs,v,bp,hybrid>" << endl;
    if(app.compare("n_path") == 0){
      cout << "\tOPTIONAL: --start_node=<start node, default is higest degree> --n=<path length, default finds all paths>" << endl;
      cout << "*takes a directed graph as input*" << endl;
    } else if(app.compare("n_clique") == 0 || app.compare("n_cycle") == 0){
      cout << "\tOPTIONAL: --n=<size of cliques or cycles to find, default is 4>" << endl;
      cout << "*takes a undirected graph as input*" << endl;
    }
    exit (0);
  }
  Parser parse(int argc, char* argv[], string app){ 
    char* graph_path = NULL;
    char* attribute_path = NULL;
    char* input_type = NULL;
    char* layout_type = NULL;
    int num_threads = 0;
    int n = -1;
    long start_node = -1;
    bool help = false;

    int c;
    while (1){
      static struct option long_options[] = {
          /* These options don’t set a flag.
             We distinguish them by their indices. */
          {"help",required_argument,0,'h'},
          {"attributes",required_argument,0,'a'},
          {"t",required_argument,0,'t'},
          {"graph",required_argument,0,'g'},
          {"layout",required_argument,0,'l'},
          {"n",required_argument,0,'n'},
          {"start_node",required_argument,0,'s'},
          {"input_type",  required_argument, 0, 'f'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "a:t:g:n:f:s:h:l",long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c){
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;
        case 'a':
          attribute_path = optarg;
          break;
        case 'h':
          help = true;
          break;
        case 't':
          num_threads = atoi(optarg);
          break;
        case 'g':
          graph_path = optarg;
          break;
        case 'f':
          input_type = optarg;
          break;
        case 'l':
          layout_type = optarg;
          break;
        case 'n':
          n = atoi(optarg);
          break;
        case 's':
          start_node = atol(optarg);
          break;
        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          abort ();
        }
    }
    if(num_threads == 0 || graph_path == NULL || input_type == NULL || help || layout_type == NULL){
      printUsage(app);
    }
    
    if(n == -1){
      if(app.compare("n_clique") == 0 || app.compare("n_cycle") == 0){
        n = 4; //default is 4
      }else if(app.compare("n_path") == 0){
        n = 0xeffffff;
      }
    }

    MutableGraph *inputGraph;
    if(app.compare("n_path") == 0){
      if(attribute_path != NULL){
        #ifndef ATTRIBUTES
        cout << "WARNING: Gave a attributes file but pragma is turned off." << endl;
        #endif 
        inputGraph = MutableGraph::directedFromAttributeList(graph_path,attribute_path);
      } else{
        if(string(input_type).compare("text") == 0){
          inputGraph = MutableGraph::directedFromEdgeList(graph_path);
        } else{
          inputGraph = MutableGraph::directedFromBinary(graph_path);
        }
      }
    } else{
      if(attribute_path != NULL){
        #ifndef ATTRIBUTES
        cout << "WARNING: Gave a attributes file but pragma is turned off." << endl;
        #endif         
        inputGraph = MutableGraph::undirectedFromAttributeList(graph_path,attribute_path);
      } else{
        if(string(input_type).compare("text") == 0){
          inputGraph = MutableGraph::undirectedFromEdgeList(graph_path);
        } else{
          inputGraph = MutableGraph::undirectedFromBinary(graph_path);
        }
      }
    }
    return Parser(num_threads,attribute_path!=NULL,n,start_node,inputGraph,layout_type);
  }
}
