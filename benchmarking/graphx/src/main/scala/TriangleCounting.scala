import org.apache.spark.SparkContext
import org.apache.spark.SparkContext._
import org.apache.spark.SparkConf

import org.apache.spark._
import org.apache.spark.graphx._
import org.apache.spark.rdd.RDD

import scala.collection.mutable.Set

import scala.reflect.ClassTag

object TriangleCounting {
  def main(args: Array[String]) {
    val conf = new SparkConf()
      .setAppName("GraphX Clique Counting")
      .set("spark.executor.memory", "2g")
    val sc = new SparkContext(conf)
    val edges = GraphLoader.edgeListFile(sc, args(0))

    val startTime = System.nanoTime()
    val triangles = edges.triangleCount()
    val result = triangles.vertices.reduce((a: (VertexId, Int), b: (VertexId, Int)) => (a._1, a._2 + b._2))
    println(result._2)
    println("Time: " + (System.nanoTime() - startTime))
  }
}
