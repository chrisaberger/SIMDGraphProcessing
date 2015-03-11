SET SESSION RESOURCE_POOL = benchmarking_pool;

\timing
-- Lollipop counting
SELECT COUNT(*) FROM
  uedges e1 JOIN uedges e2 ON e1.dest = e2.source AND e2.source > e2.dest JOIN uedges e3 ON e2.dest = e3.dest AND e1.source = e3.source JOIN uedges e4 ON e1.source = e4.source;
\timing
