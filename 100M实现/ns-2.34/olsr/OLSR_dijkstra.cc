 /***************************************************************************
  *   Copyright (C) 2004 by Francisco J. Ros                                *
  *   fjrm@dif.um.es                                                        *
  *                                                                         *
  *   Modified by Weverton Cordeiro                                         *
  *   (C) 2007 wevertoncordeiro@gmail.com                                   *
  *                                                                         *
  *   This program is free software; you can redistribute it and/or modify  *
  *   it under the terms of the GNU General Public License as published by  *
  *   the Free Software Foundation; either version 2 of the License, or     *
  *   (at your option) any later version.                                   *
  *                                                                         *
  *   This program is distributed in the hope that it will be useful,       *
  *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
  *   GNU General Public License for more details.                          *
  *                                                                         *
  *   You should have received a copy of the GNU General Public License     *
  *   along with this program; if not, write to the                         *
  *   Free Software Foundation, Inc.,                                       *
  *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
  ***************************************************************************/
 
 /*** New File Added ***/
 
 #include <olsr/OLSR_dijkstra.h>
 #include <olsr/OLSR.h>
 
 Dijkstra::Dijkstra()
 {
   highest_hop_ = 0;
 
   D_ = new std::map<nsaddr_t, hop * >();
   nonprocessed_nodes_ = new std::set<nsaddr_t>();
   link_array_ = new map<nsaddr_t, vector<edge*> * > ();
   all_nodes_ = new std::set<nsaddr_t> ();
 }
 
 void Dijkstra::add_edge (nsaddr_t dest_node, nsaddr_t last_node, double delay,
                           double quality, bool direct_connected)
 {
   edge *link = new edge;
 
   // The last hop is the interface in which we have this neighbor...
   link->last_node() = last_node;
   // Also record the link delay and quality..
   link->delay() = delay;
   link->quality() = quality;
 
   if ((*link_array_)[dest_node] == NULL)
     (*link_array_)[dest_node] = new vector<edge*> ();
   (*link_array_)[dest_node]->push_back(link);
 
   if (direct_connected) {
     // Since dest_node is directly connected to the node running the djiksra
     // algorithm, this link has hop count 1
     (*D_)[dest_node] = new hop;
     (*D_)[dest_node]->hop_count() = 1;
     // Report the best link we have at the moment to these nodes
     (*D_)[dest_node]->link().last_node() = link->last_node();
     (*D_)[dest_node]->link().delay() = link->delay();
     (*D_)[dest_node]->link().quality() = link->quality();
   } else if ((*all_nodes_).find(dest_node) == (*all_nodes_).end()) {
     // Since we don't have an edge connecting dest_node to the node running
     // the dijkstra algorithm, the cost is set to infinite...
     (*D_)[dest_node] = new hop;
     (*D_)[dest_node]->hop_count() = -1;
   }
 
   // Add dest_node to the list of nodes we will have to iterate through
   // while calculating the best path among nodes...
   (*all_nodes_).insert(dest_node);
 
   // Add dest_node to the list of nodes we will have to process...
   (*nonprocessed_nodes_).insert(dest_node);
 }
 
 set<nsaddr_t>::iterator Dijkstra::best_cost ()
 {
   set<nsaddr_t>::iterator best = (*nonprocessed_nodes_).end();
 
   // Search for a node that was not processed yet and that has
   // the best cost to be reached from the node running dijkstra...
   for (set<nsaddr_t>::iterator it = (*nonprocessed_nodes_).begin();
         it != (*nonprocessed_nodes_).end(); it++) {
     if ((*D_)[*it]->hop_count() == -1) // there is no such link yet, i. e., it's cost is infinite...
       continue;
     if (best == (*nonprocessed_nodes_).end())
       best = it;
     else {
       if (parameter_.link_delay()) {
         /// Link quality extension
         if ((*D_)[*it]->link().delay() < (*D_)[*best]->link().delay())
           best = it;
       } else {
         switch (parameter_.link_quality()) {
         case OLSR_BEHAVIOR_ETX:
           if ((*D_)[*it]->link().quality() < (*D_)[*best]->link().quality())
             best = it;
           break;
 
         case OLSR_BEHAVIOR_ML:
           if ((*D_)[*it]->link().quality() > (*D_)[*best]->link().quality())
             best = it;
           break;
 
         case OLSR_BEHAVIOR_NONE:
         default:
           //
           break;
         }
       }
     }
   }
   return best;
 }
 
 edge* Dijkstra::get_edge (nsaddr_t dest_node, nsaddr_t last_node)
 {
   // Find the edge that connects dest_node and last_node
   for (std::vector<edge*>::iterator it = (*link_array_)[dest_node]->begin();
        it != (*link_array_)[dest_node]->end(); it++) {
     edge* current_edge = *it;
     if (current_edge->last_node() == last_node)
       return current_edge;
   }
   return NULL;
 }
 
 void Dijkstra::run()
 {
   // While there are non processed nodes...
   while ((*nonprocessed_nodes_).begin() != (*nonprocessed_nodes_).end()) {
     // Get the node among those nom processed having best cost...
     set<nsaddr_t>::iterator current_node = best_cost();
     // If all non processed nodes have cost equals to infinite, there is
     // nothing left to do, but abort (this might be the case of a not
     // fully connected graph)
     if (current_node == (*nonprocessed_nodes_).end())
       break;
 
     // for each node in all_nodes...
     for (set<nsaddr_t>::iterator dest_node = (*all_nodes_).begin();
           dest_node != (*all_nodes_).end(); dest_node++) {
       // ... not processed yet...
       if ((*nonprocessed_nodes_).find(*dest_node) == (*nonprocessed_nodes_).end())
         continue;
       // .. and adjacent to 'current_node'...
       // note: edge has destination '*dest_node' and last hop '*current_node'
       edge* current_edge = get_edge(*dest_node, *current_node);
       if (current_edge == NULL)
         continue;
       // D(node) = min (D(node), D(current_node) + edge(current_node, node).cost())
       if ((*D_)[*dest_node]->hop_count() == -1) { // there is not a link to dest_node yet...
         switch (parameter_.link_quality()) {
         case OLSR_BEHAVIOR_ETX:
           (*D_)[*dest_node]->link().last_node() = current_edge->last_node();
           (*D_)[*dest_node]->link().quality() = (*D_)[*current_node]->link().quality() + current_edge->quality();
           /// Link delay extension
           (*D_)[*dest_node]->link().delay() = (*D_)[*current_node]->link().delay() + current_edge->delay();
           (*D_)[*dest_node]->hop_count() = (*D_)[*current_node]->hop_count() + 1;
           // Keep track of the highest path we have by means of number of hops...
           if ((*D_)[*dest_node]->hop_count() > highest_hop_)
             highest_hop_ = (*D_)[*dest_node]->hop_count();
           break;
 
         case OLSR_BEHAVIOR_ML:
           (*D_)[*dest_node]->link().last_node() = current_edge->last_node();
           (*D_)[*dest_node]->link().quality() = (*D_)[*current_node]->link().quality() * current_edge->quality();
           /// Link delay extension
           (*D_)[*dest_node]->link().delay() = (*D_)[*current_node]->link().delay() + current_edge->delay();
           (*D_)[*dest_node]->hop_count() = (*D_)[*current_node]->hop_count() + 1;
           // Keep track of the highest path we have by means of number of hops...
           if ((*D_)[*dest_node]->hop_count() > highest_hop_)
             highest_hop_ = (*D_)[*dest_node]->hop_count();
           break;
 
         case OLSR_BEHAVIOR_NONE:
         default:
           //
           break;
         }
       } else {
         if (parameter_.link_delay()) {
           /// Link delay extension
           switch (parameter_.link_quality()) {
           case OLSR_BEHAVIOR_ETX:
             if ((*D_)[*current_node]->link().delay() + current_edge->delay() < (*D_)[*dest_node]->link().delay()) {
               (*D_)[*dest_node]->link().last_node() = current_edge->last_node();
               (*D_)[*dest_node]->link().quality() = (*D_)[*current_node]->link().quality() + current_edge->quality();
               (*D_)[*dest_node]->link().delay() = (*D_)[*current_node]->link().delay() + current_edge->delay();
               (*D_)[*dest_node]->hop_count() = (*D_)[*current_node]->hop_count() + 1;
               // Keep track of the highest path we have by means of number of hops...
               if ((*D_)[*dest_node]->hop_count() > highest_hop_)
                 highest_hop_ = (*D_)[*dest_node]->hop_count();
             }
             break;
 
           case OLSR_BEHAVIOR_ML:
             if ((*D_)[*current_node]->link().delay() + current_edge->delay() < (*D_)[*dest_node]->link().delay()) {
               (*D_)[*dest_node]->link().last_node() = current_edge->last_node();
               (*D_)[*dest_node]->link().quality() = (*D_)[*current_node]->link().quality() * current_edge->quality();
               (*D_)[*dest_node]->link().delay() = (*D_)[*current_node]->link().delay() + current_edge->delay();
               (*D_)[*dest_node]->hop_count() = (*D_)[*current_node]->hop_count() + 1;
               // Keep track of the highest path we have by means of number of hops...
               if ((*D_)[*dest_node]->hop_count() > highest_hop_)
                 highest_hop_ = (*D_)[*dest_node]->hop_count();
             }
             break;
 
           case OLSR_BEHAVIOR_NONE:
           default:
             //
             break;
           }
         } else {
           switch (parameter_.link_quality()) {
           case OLSR_BEHAVIOR_ETX:
             if ((*D_)[*current_node]->link().quality() + current_edge->quality() < (*D_)[*dest_node]->link().quality()) {
               (*D_)[*dest_node]->link().last_node() = current_edge->last_node();
               (*D_)[*dest_node]->link().quality() = (*D_)[*current_node]->link().quality() + current_edge->quality();
               (*D_)[*dest_node]->hop_count() = (*D_)[*current_node]->hop_count() + 1;
               // Keep track of the highest path we have by means of number of hops...
               if ((*D_)[*dest_node]->hop_count() > highest_hop_)
                 highest_hop_ = (*D_)[*dest_node]->hop_count();
             }
             break;
 
           case OLSR_BEHAVIOR_ML:
             if ((*D_)[*current_node]->link().quality() * current_edge->quality() > (*D_)[*dest_node]->link().quality()) {
               (*D_)[*dest_node]->link().last_node() = current_edge->last_node();
               (*D_)[*dest_node]->link().quality() = (*D_)[*current_node]->link().quality() * current_edge->quality();
               (*D_)[*dest_node]->hop_count() = (*D_)[*current_node]->hop_count() + 1;
               // Keep track of the highest path we have by means of number of hops...
               if ((*D_)[*dest_node]->hop_count() > highest_hop_)
                 highest_hop_ = (*D_)[*dest_node]->hop_count();
             }
             break;
 
           case OLSR_BEHAVIOR_NONE:
           default:
             //
             break;
           }
         }
       }
     }
     // Remove it from the list of processed nodes
     (*nonprocessed_nodes_).erase(current_node);
   }
 }
 
 Dijkstra::~Dijkstra()
 {
   for (map<nsaddr_t, vector<edge*>*>::iterator it = (*link_array_).begin(); it != (*link_array_).end(); it++) {
     vector<edge*> * v = (*it).second;
     for (vector<edge*>::iterator it2 = (*v).begin(); it2 != (*v).end(); it2++)
       delete *it2;
     delete (*it).second;
   }
 
   for (map<nsaddr_t, hop * >::iterator it = D_->begin(); it != D_->end(); it++)
     delete (*it).second;
   delete D_;
 
   delete link_array_;
   delete nonprocessed_nodes_;
   delete all_nodes_;
 }

