SET SESSION RESOURCE_POOL = benchmarking_pool;

\timing
-- Clique counting
SELECT COUNT(*) FROM
   edges e1 JOIN edges e2 ON e1.dest = e2.source JOIN edges e3 ON e2.dest = e3.source JOIN edges e4 ON e3.dest = e4.dest AND e4.source = e1.source
   JOIN edges e5 ON e1.source = e5.source AND e2.dest = e5.dest JOIN edges e6 ON e1.dest = e6.source AND e3.dest = e6.dest;
\timing
