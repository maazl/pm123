svn status >stat
grep -E -i -v "\?.*(\.(dll|exe|o|lib|a|bmp|res)|makedebug\.cmd|/diff)$" stat >stat2