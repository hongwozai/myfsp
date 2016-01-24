create database fsp character set utf8;
use fsp;
create table usr(
        name varchar(30) primary key,
        passwd varchar(30) not null
        );
create table share(
        fromusr varchar(30),
        tousr varchar(30),
        filename varchar(54),
        isdir varchar(1) check(isdir in ('t', 'f')),
        filepath varchar(128),
        primary key (fromusr, tousr, filename),
        foreign key (fromusr) references usr(name),
        foreign key (tousr)   references usr(name)
        );
insert into usr values('all', 'all');
