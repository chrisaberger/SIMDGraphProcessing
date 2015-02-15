DROP TABLE edges;
DROP TABLE uedges;
CREATE TABLE edges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source, dest) ALL NODES;
CREATE TABLE uedges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source, dest) ALL NODES;

copy edges from :file direct delimiter ' ';

\timing
-- Clique counting
SELECT COUNT(*) FROM
   edges e1 JOIN edges e2 ON e1.dest = e2.source JOIN edges e3 ON e2.dest = e3.source JOIN edges e4 ON e3.source = e4.dest AND e4.source = e1.source
   JOIN edges e5 ON e1.source = e5.source AND e2.dest = e5.dest JOIN edges e6 ON e1.dest = e6.source AND e3.dest = e6.dest;
\timing
