import org.apache.spark.SparkContext
import org.apache.spark.SparkContext._
import org.apache.spark.SparkConf

import org.apache.spark._
import org.apache.spark.graphx._
import org.apache.spark.rdd.RDD

import scala.collection.mutable.Set

import scala.reflect.ClassTag

object CliqueCounting {
  def run[VD: ClassTag, ED: ClassTag](graph: Graph[VD,ED]): Graph[Int, ED] = {
    // Remove redundant edges (do we need this step in our case?)
    val g = graph.groupEdges((a, b) => a).cache()

    // Turn edges into "two-cliques"
    val edges: VertexRDD[Set[Set[VertexId]]] =
      g.collectNeighborIds(EdgeDirection.Either).mapValues { (vid, nbrs) =>
        val set = Set[Set[VertexId]]()
        var i = 0
        while (i < nbrs.size) {
          // prevent self cycle
          if(nbrs(i) != vid) {
            set.add(Set(nbrs(i)))
          }
          i += 1
        }
        set
      }

    def findCliques(cliqueSets: VertexRDD[Set[Set[VertexId]]], currSize: Int, tgtSize: Int): VertexRDD[Set[Set[VertexId]]] = {
      if(currSize == tgtSize)
        cliqueSets
      else {
        // Join the cliques with the graph
        val setGraph: Graph[Set[Set[VertexId]], ED] = g.outerJoinVertices(cliqueSets) {
          (vid, _, optSet) => optSet.getOrElse(null)
        }

        // Edge function computes intersection of smaller vertex with larger vertex
        def edgeFunc(et: EdgeTriplet[Set[Set[VertexId]], ED]): Iterator[(VertexId, Set[Set[VertexId]])] = {
          assert(et.srcAttr != null)
          assert(et.dstAttr != null)
          val (smallSet, largeSet) = if (et.srcAttr.size < et.dstAttr.size) {
            (et.srcAttr, et.dstAttr)
          } else {
            (et.dstAttr, et.srcAttr)
          }
          val iter = smallSet.iterator
          var cliques: Set[Set[VertexId]] = Set()
          while (iter.hasNext) {
            val clique = iter.next()
            if (!clique.contains(et.dstId) && largeSet.contains(clique)) {
              cliques.add(clique + et.dstId)
            }
          }
          Iterator((et.srcId, cliques), (et.dstId, cliques))
        }

        // Compute the intersection of cliques along edges
        findCliques(setGraph.mapReduceTriplets(edgeFunc, _ ++ _), currSize + 1, tgtSize)
      }
    }

    val cliques = findCliques(edges, 2, 4)

    // Merge counters with the graph and divide by two since each triangle is counted twice
    g.outerJoinVertices(cliques) {
      (vid, _, optVertCliques: Option[Set[Set[VertexId]]]) =>
        val vertCliques = optVertCliques.getOrElse(Set())
        vertCliques.size
    }
  }

  def main(args: Array[String]) {
    val conf = new SparkConf()
      .setAppName("GraphX Clique Counting")
      .set("spark.executor.memory", "2g")
    val sc = new SparkContext(conf)
    val edges = GraphLoader.edgeListFile(sc, "/dfs/scratch0/noetzli/datasets/california/edgelist/data.txt")
    val triangles = run(edges)
    val result = triangles.vertices.reduce((a: (VertexId, Int), b: (VertexId, Int)) => (a._1, a._2 + b._2))
    println(result._2)
  }
}
