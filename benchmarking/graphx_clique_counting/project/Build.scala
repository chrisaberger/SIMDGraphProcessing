import sbt._
import Keys._

object GraphXCliqueCountingBuild extends Build {
  val hwsettings = Defaults.defaultSettings

  val submit = TaskKey[Unit]("submit", "Submits the package to Spark")

  val submitTask = submit := {
    println("Hello World")
  }

  lazy val project = Project (
    "project",
    file ("."),
    settings = hwsettings ++ Seq(submitTask)
  )
}
