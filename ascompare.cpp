//   Copyright 2008-2013 Kristopher R Beevers and Internap Network
//   Services Corporation.

//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:

//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.

//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

using namespace boost;

typedef adjacency_list<listS, vecS, undirectedS, no_property,
                       property <edge_weight_t, int> > graph_t;
typedef graph_traits <graph_t>::vertex_descriptor vertex_descriptor;
typedef graph_traits <graph_t>::edge_descriptor edge_descriptor;


struct as_t
{
  std::string name;
  uint32_t cone_24s;
  uint32_t cone_prefixes;
  uint32_t cone_ases;
};

struct as_stats_t
{
  const char *label;
  std::vector<uint32_t> asns;           // asns in a group
  std::vector<uint32_t> d_hist, d_cum;  // route distance histogram (by hops)
  std::vector<uint32_t> r_hist, r_cum;  // reachable ips histogram  (by hops)
  uint32_t d_total, r_total, d_max, r_max;

  as_stats_t()
    : label(0), d_hist(100), d_cum(100), r_hist(100), r_cum(100),
      d_total(0), r_total(0), d_max(0), r_max(0) {}
};

class reachable_visitor : public dijkstra_visitor<null_visitor>
{
public:
  reachable_visitor(const std::map<uint32_t, as_t> &_AS,
                    std::vector<int> &_d, std::vector<uint32_t> &_r)
    : AS(_AS), d(_d), reachable(_r) {}

  template <class Vertex, class Graph>
  void finish_vertex(Vertex u, Graph &g)
  {
    std::map<uint32_t, as_t>::const_iterator as = AS.find(u);
    if(as != AS.end())
      reachable[d[u]] += as->second.cone_24s;
  }
protected:
  const std::map<uint32_t, as_t> &AS;
  std::vector<int> &d;
  std::vector<uint32_t> &reachable;
};


graph_t g;
std::map<uint32_t, as_t> AS;



void compute_as_stats(const graph_t &g, uint32_t asn, as_stats_t &S)
{
  std::vector<vertex_descriptor> p(num_vertices(g));
  std::vector<int> d(num_vertices(g));
  reachable_visitor rv(AS, d, S.r_hist);
  printf("computing shortest paths for %s...", S.label);
  dijkstra_shortest_paths(g, vertex(asn, g), predecessor_map(&p[0]).distance_map(&d[0]).visitor(rv));
  printf("done.\n");

  graph_traits<graph_t>::vertex_iterator vi, vi_end;
  for(tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    if(d[*vi] < 100)
      ++S.d_hist[d[*vi]];
  }

  for(uint32_t i = 0; i < 100; ++i) {
    S.d_cum[i] = S.d_hist[i] + ((i > 0) ? S.d_cum[i-1] : 0);
    S.r_cum[i] = S.r_hist[i] + ((i > 0) ? S.r_cum[i-1] : 0);
    S.d_total += S.d_hist[i];
    S.r_total += S.r_hist[i];
    if(S.d_hist[i] > S.d_max) S.d_max = S.d_hist[i];
    if(S.r_hist[i] > S.r_max) S.r_max = S.r_hist[i];
  }
}

void print_hops_as_header(const std::vector<as_stats_t> &S)
{
  printf("%4s", "hops");
  for(uint32_t i = 0; i < S.size(); ++i) {
    printf("%16s", S[i].label);
  }
  printf("\n");
}

int main(int argc, char **argv)
{
  if(argc < 4) {
    fprintf(stderr, "Usage: %s <edge-file> <AS-list> <LABEL:asn1,asn2,...> <LABEL:asn1,...> ...\n", argv[0]);
    return 1;
  }

  printf("CMDLINE: ");
  for(int i = 0; i < argc; ++i)
    printf("%s ", argv[i]);
  printf("\n");

  printf("loading AS graph...");
  FILE *in = fopen(argv[1], "r");
  uint32_t from, to, weight;
  edge_descriptor e;
  bool inserted;
  property_map<graph_t, edge_weight_t>::type weightmap = get(edge_weight, g);
  while(fscanf(in, "%u %u %u", &from, &to, &weight) == 3) {
    tie(e, inserted) = add_edge(from, to, g);
    weightmap[e] = weight;
  }
  fclose(in);
  printf("done.\n");

  printf("loading AS list...");
  in = fopen(argv[2], "r");
  uint32_t asn;
  char name[256];
  as_t as;
  while(fscanf(in, "%u %255s %u %u %u", &asn, name,
               &as.cone_24s, &as.cone_prefixes, &as.cone_ases) == 5) {
    as.name = std::string(name);
    AS[asn] = as;
  }
  fclose(in);
  printf("done.\n");

  std::vector<as_stats_t> as_stats;
  for(int i = 3; i < argc; ++i) {
    // allow groups of AS's to be treated as one effective AS using
    // the following format: LABEL:asn1,asn2,asn3,...
    as_stats.push_back(as_stats_t());
    as_stats_t &S = as_stats.back();
    char *c = strchr(argv[i], ':'), *m;
    if(c) {
      *c = '\0';
      ++c;
    } else
      c = argv[i];
    S.label = argv[i];
    printf("GROUP %20s: ", S.label);
    while(c) {
      if((m = strchr(c, ',')))
        *m = '\0';
      S.asns.push_back(strtoul(c, 0, 10));
      printf("%6u  ", S.asns.back());
      if(m)
        c = m + 1;
      else
        break;
    }
    printf("\n");

    // if there are multiple ASNs that we want to treat as one
    // effective AS, put edges of weight 0 between all of them in the
    // graph
    if(S.asns.size() > 1) {
      for(uint32_t j = 0; j < S.asns.size(); ++j)
        for(uint32_t k = j+1; k < S.asns.size(); ++k) {
          tie(e, inserted) = add_edge(S.asns[j], S.asns[k], g);
          weightmap[e] = 0;
        }
    }
  }

  uint32_t d_max = 0, r_max = 0;
  for(uint32_t i = 0; i < as_stats.size(); ++i) {
    compute_as_stats(g, as_stats[i].asns[0], as_stats[i]);
    if(as_stats[i].d_max > d_max)
      d_max = as_stats[i].d_max;
    if(as_stats[i].r_max > r_max)
      r_max = as_stats[i].r_max;
  }

  for(uint32_t i = 0; i < as_stats.size(); ++i) {
    const as_stats_t &S = as_stats[i];
    for(uint32_t j = 0; j < S.asns.size(); ++j) {
      const as_t &as = AS[S.asns[j]];
      printf("%2s%6u (%s) cones: /24s %u  prefixes %u  ASes %u\n", "AS",
             S.asns[j], as.name.c_str(), as.cone_24s, as.cone_prefixes, as.cone_ases);
    }
  }

  printf("NAME: reachable AS histogram\n");
  printf("OUTPUT: as-hist\n");
  printf("MAX: %u\n", d_max);
  print_hops_as_header(as_stats);
  bool something = true;
  for(uint32_t i = 0; i < 100 && something; ++i) {
    something = false;
    printf("%4u", i);
    for(uint32_t j = 0; j < as_stats.size(); ++j) {
      const as_stats_t &S = as_stats[j];
      printf("%16u", S.d_hist[i]);
      if(S.d_hist[i] > 0)
        something = true;
    }
    printf("\n");
  }

  printf("NAME: reachable AS cumulative distribution\n");
  printf("OUTPUT: as-cum\n");
  printf("MAX: 1.0\n");
  print_hops_as_header(as_stats);
  something = true;
  uint32_t d_tot_max = 0;
  for(uint32_t j = 0; j < as_stats.size(); ++j)
    if(as_stats[j].d_total > d_tot_max)
      d_tot_max = as_stats[j].d_total;
  for(uint32_t i = 0; i < 100 && something; ++i) {
    something = false;
    printf("%4u", i);
    for(uint32_t j = 0; j < as_stats.size(); ++j) {
      const as_stats_t &S = as_stats[j];
      printf("%9s%1.5f", "", float(S.d_cum[i]) / float(d_tot_max));
      if(S.d_hist[i] > 0)
        something = true;
    }
    printf("\n");
  }

  printf("NAME: reachable /24s histogram\n");
  printf("OUTPUT: 24-hist\n");
  printf("MAX: %u\n", r_max);
  print_hops_as_header(as_stats);
  something = true;
  for(uint32_t i = 0; i < 100 && something; ++i) {
    something = false;
    printf("%4u", i);
    for(uint32_t j = 0; j < as_stats.size(); ++j) {
      const as_stats_t &S = as_stats[j];
      printf("%16u", S.r_hist[i]);
      if(S.r_hist[i] > 0)
        something = true;
    }
    printf("\n");
  }

  printf("NAME: reachable /24s cumulative distribution\n");
  printf("OUTPUT: 24-cum\n");
  printf("MAX: 1.0\n");
  print_hops_as_header(as_stats);
  something = true;
  uint32_t r_tot_max = 0;
  for(uint32_t j = 0; j < as_stats.size(); ++j)
    if(as_stats[j].r_total > r_tot_max)
      r_tot_max = as_stats[j].r_total;
  for(uint32_t i = 0; i < 100 && something; ++i) {
    something = false;
    printf("%4u", i);
    for(uint32_t j = 0; j < as_stats.size(); ++j) {
      const as_stats_t &S = as_stats[j];
      printf("%9s%1.5f", "", float(S.r_cum[i]) / float(r_tot_max));
      if(S.r_hist[i] > 0)
        something = true;
    }
    printf("\n");
  }

  return 0;
}
