-- 100yd in continent maps
UPDATE `worldmap_info` SET `viewingDistance`='100' WHERE `type`='0';
-- 200yd in dungeons and raids
UPDATE `worldmap_info` SET `viewingDistance`='200' WHERE `type`='1' OR `type`='2' OR `type`='4';
-- 500yd in battlegrounds
UPDATE `worldmap_info` SET `viewingDistance`='500' WHERE `type`='3';

-- TODO: update ID and date!
INSERT INTO `world_db_version` VALUES ('100', '20211026-00_worldmap_info');
