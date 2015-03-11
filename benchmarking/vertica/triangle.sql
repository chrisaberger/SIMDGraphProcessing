DROP TABLE edges;
DROP TABLE uedges;
CREATE TABLE edges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source, dest) ALL NODES;
CREATE TABLE uedges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source, dest) ALL NODES;

copy edges from :file direct delimiter ' ';

\timing
-- Triangle counting
SELECT COUNT(*) FROM
   edges e1 JOIN edges e2 ON e1.dest = e2.source JOIN edges e3 ON e2.dest = e3.dest AND e3.source = e1.source;
\timing
