--2014-08-20


-- alter the series view wrt new changes in apps

delete from views where id=7;

INSERT INTO views VALUES(7,7,1,'By Series',2,'SELECT DISTINCT series,max(read_date) as series_read_date,sum(read_count) as series_read_count, count(*) as total_books, (select count(*) from books c where c.series=b.series and read_count = 0) as series_unread, (select count(*) from books c where c.series=b.series and read_count > 0) as series_read FROM books b WHERE series <> '''' group by upper(series) ORDER BY read_date desc, upper(series), series;SELECT DISTINCT file, title, author,read_count, series, series_index FROM books WHERE series = :series ORDER BY series_index','/usr/share/explorer/images/middle/series.png');



-- add cr3 as optional mobi reader; untested
DELETE FROM associations WHERE id=4;
INSERT INTO associations VALUES(4,'mobi',10,'10;11',NULL,'/usr/share/explorer/images/middle/mobi.png',2)

delete from views where id=3;
delete from views where id=4;

INSERT INTO views VALUES(3,3,1,'By Title',2,'SELECT DISTINCT file, title, author,read_count, series_index, series  FROM books ORDER BY upper(title), title','/usr/share/explorer/images/middle/dictionary.png');
INSERT INTO views VALUES(4,4,1,'By Author',2,'SELECT DISTINCT author FROM books ORDER BY upper(author), author;SELECT DISTINCT file, title, author,read_count,series,series_index FROM books WHERE author = :author ORDER BY upper(title), title','/usr/share/explorer/images/middle/author.png');


