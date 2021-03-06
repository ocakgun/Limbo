/**
 * @file   test_MISColoring.cpp
 * @brief  test MIS based coloring algorithm @ref limbo::algorithms::coloring::MISColoring
 * @author Yibo Lin
 * @date   Feb 2015
 */

#include <iostream>
#include <fstream>
#include <string>
#include <limbo/preprocessor/AssertMsg.h>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <limbo/algorithms/coloring/MISColoring.h>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/iteration_macros.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION <= 14601
#include <boost/graph/detail/is_same.hpp>
#else
#include <boost/type_traits/is_same.hpp>
#endif

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::pair;
using namespace boost;

/// @nowarn 
// do not use setS, it does not compile for subgraph
// do not use custom property tags, it does not compile for most utilities
typedef adjacency_list<vecS, vecS, undirectedS, 
		property<vertex_index_t, std::size_t, property<vertex_color_t, int> >, 
		property<edge_index_t, std::size_t, property<edge_weight_t, int> >,
		property<graph_name_t, string> > graph_type;
typedef subgraph<graph_type> subgraph_type;
typedef property<vertex_index_t, std::size_t> VertexId;
typedef property<edge_index_t, std::size_t> EdgeID;
typedef graph_traits<graph_type>::vertex_descriptor vertex_descriptor; 
typedef graph_traits<graph_type>::edge_descriptor edge_descriptor; 
typedef property_map<graph_type, edge_weight_t>::type edge_weight_map_type;
typedef property_map<graph_type, vertex_color_t>::type vertex_color_map_type;
/// @endnowarn

/// test 1: a random graph 
void randomGraph() 
{
	mt19937 gen;
	graph_type g;
	int N = 40;
	std::vector<vertex_descriptor> vertex_set;
	std::vector< std::pair<vertex_descriptor, vertex_descriptor> > edge_set;
	generate_random_graph(g, N, N * 2, gen,
			std::back_inserter(vertex_set),
			std::back_inserter(edge_set));
	BOOST_AUTO(edge_weight_map, get(edge_weight, g));
	unsigned int i = 0; 
	graph_traits<graph_type>::edge_iterator eit, eit_end;
	for (tie(eit, eit_end) = edges(g); eit != eit_end; ++eit, ++i)
	{
        edge_weight_map[*eit] = 1;
	}

	//test relaxed LP based coloring
	limbo::algorithms::coloring::MISColoring<graph_type> lc (g); 
	lc.stitch_weight(0.1);
	// THREE or FOUR 
	lc.color_num(limbo::algorithms::coloring::MISColoring<graph_type>::THREE);
	double cost = lc();
    cout << "final cost = " << cost << endl;
}

/// test 2: a real graph from input 
/// @param filename input file in graphviz format 
void realGraph(string const& filename)
{
	ifstream in (filename.c_str());

	// the graphviz reader in boost cannot specify vertex_index_t
	// I have to create a temporary graph and then construct the real graph 
	typedef adjacency_list<vecS, vecS, undirectedS, 
			property<vertex_index_t, std::size_t, property<vertex_color_t, int, property<vertex_name_t, std::size_t> > >, 
			property<edge_index_t, std::size_t, property<edge_weight_t, int> >,
			property<graph_name_t, string> > tmp_graph_type;
	tmp_graph_type tmpg;
	dynamic_properties tmpdp;
	tmpdp.property("node_id", get(vertex_name, tmpg));
	tmpdp.property("label", get(vertex_name, tmpg));
	tmpdp.property("weight", get(edge_weight, tmpg));
	tmpdp.property("label", get(edge_weight, tmpg));
	limboAssert(read_graphviz(in, tmpg, tmpdp, "node_id"));

	// real graph 
	graph_type g (num_vertices(tmpg));
	graph_traits<tmp_graph_type>::vertex_iterator vit, vit_end;
	for (tie(vit, vit_end) = vertices(tmpg); vit != vit_end; ++vit)
	{
		size_t name  = get(vertex_name, tmpg, *vit);
		int color = get(vertex_color, tmpg, *vit);
		put(vertex_color, g, (vertex_descriptor)name, color);
	}

	graph_traits<tmp_graph_type>::edge_iterator eit, eit_end;
	for (tie(eit, eit_end) = edges(tmpg); eit != eit_end; ++eit)
	{
		size_t s_name = get(vertex_name, tmpg, source(*eit, tmpg));
		size_t t_name = get(vertex_name, tmpg, target(*eit, tmpg));
		pair<edge_descriptor, bool> pe = add_edge(s_name, t_name, g);
		limboAssert(pe.second);
		int weight = get(edge_weight, g, *eit);
		put(edge_weight, g, pe.first, weight);
	}

#ifdef DEBUG_MISCOLORING
	dynamic_properties dp;
	dp.property("id", get(vertex_index, g));
	dp.property("node_id", get(vertex_index, g));
	dp.property("label", get(vertex_index, g));
	dp.property("weight", get(edge_weight, g));
	dp.property("label", get(edge_weight, g));
	ofstream out ("graph_init.gv");
	write_graphviz_dp(out, g, dp, string("id"));
	out.close();
	system("dot -Tpdf graph_init.gv -o graph_init.pdf");
#endif

	//test relaxed LP based coloring
	limbo::algorithms::coloring::MISColoring<graph_type> lc (g); 
    // stitch is not supported 
	// THREE or FOUR 
	lc.color_num(limbo::algorithms::coloring::MISColoring<graph_type>::FOUR);
	double cost = lc();
    cout << "final cost = " << cost << endl;

	in.close();
}

/// main function \n
/// test either on random graph or real graph 
/// @param argc number of arguments 
/// @param argv values of arguments 
/// @return 0 
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		randomGraph();
	}
	else realGraph(argv[1]);

	return 0;
}
