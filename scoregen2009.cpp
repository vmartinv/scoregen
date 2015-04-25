#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream> 
#include <algorithm>
#include <unistd.h>
#include <htmlcxx/html/ParserDom.h>
using namespace std;
#define dprint(v) cerr << #v"=" << v << endl //;)
#define forr(i,a,b) for(int i=(a); i<(b); i++)
#define forn(i,n) forr(i,0,n)
#define dforn(i,n) for(int i=n-1; i>=0; i--)
#define forall(it,v) for(typeof(v.begin()) it=v.begin();it!=v.end();++it)
#define sz(c) ((int)c.size())
#define zero(v) memset(v, 0, sizeof(v))
#define fst first
#define snd second
#define mkp make_pair
#define pb push_back
typedef long long ll;
typedef pair<int,int> ii;

using namespace htmlcxx;

tree<HTML::Node>::iterator findScoreboard(tree<HTML::Node>& dom) {
	tree <HTML::Node> :: iterator it = dom.begin (), end = dom.end ();
	for (;it!=end;++it) 
		if (it->tagName()=="table") {
			it-> parseAttributes ();
			if (it->attribute ("id").second == "standings")
				return it;
		}
	return it;
}

struct infocolumns {
	int firstProblem,lastProblem,team;
};

tree<HTML::Node>::iterator findProblems(tree<HTML::Node>::iterator it,tree<HTML::Node>::iterator end, infocolumns& cols) {
	cols.firstProblem=100;cols.lastProblem=-100;
	for (;it!=end;++it) if (it->tagName()=="th")  break;
	assert(it!=end);
	tree <HTML::Node> :: sibling_iterator st(it.node);
	int col = 0;
	for(;st.node!=st.range_last();++st) 
		if (st->tagName()=="th") {
			st->parseAttributes();
			if (st->attribute("class").second == "problemcolheader") {
				cols.firstProblem=min(cols.firstProblem,col);
				cols.lastProblem=max(cols.lastProblem,col);
			} else {
				string txt = st.begin()->text();
				if (txt=="Team") cols.team=col;
			}
			col++;
		}
	for(it=(it.node->parent->next_sibling);it!=end;++it) if (it->tagName()=="tr") return it;
}

ll global_time;
struct team {
	team(const string& a, const vector< pair<int,int> >& p) : name(a), problems(p) {}
	string name;
	vector< pair<int,int> > problems;
	pair<int,int> value() const {
		pair<int,int> r = make_pair(0,0);
		forall(it,problems) if (it->first!=-1 && it->second<=global_time) {
			r.first++;
			r.second -= it->second+(it->first-1)*20;
		}
		return r;
	}
};

bool operator<(const team& a, const team& b) {
	pair<int,int> va=a.value(),vb=b.value();
	return va!=vb ? va>vb : a.name<b.name;
}

struct scoreboard {
	vector< team > s;
};


void fillScoreboard(tree<HTML::Node>::iterator it,tree<HTML::Node>::iterator end,const infocolumns cols, scoreboard& score) {
	score.s.clear();
	int col;
	string teamName;
	vector< pair<int,int> > problems;
	for ( tree<HTML::Node>::sibling_iterator jt(it);jt.node!=jt.range_last();++jt) if (jt->tagName()=="tr"){
	col=0;
	for(tree <HTML::Node> :: sibling_iterator st=jt.begin() ; st != jt.end() ; ++st) {
		if (st->tagName()=="td"){
			if (col==cols.team) {
				teamName = st.begin()->text();
			} else if (col>=cols.firstProblem && col<=cols.lastProblem) {
				st->parseAttributes();
				if (st->attribute("class").second[0]=='s')
					problems.push_back( make_pair( atoi( st.node->first_child->data.text().c_str() ) , atoi ( st.node->last_child->first_child->data.text().c_str() ) ) );
				else
					problems.push_back( make_pair(-1,-1) );
			}		
		 col++;
		} else if (st->tagName()=="th") return;
	}
	score.s.push_back( team(teamName, problems) );
	problems.clear();
	}
}

int PROBLEM_FIRST_COLUMN;

void ScoreMain(char filename[], scoreboard& score) {
  fstream fs (filename, fstream::in);
  string s,html;
  while(getline(fs,s)) {
	  html += s+"\n";
  }
  fs.close();
	
  HTML::ParserDom parser;
  tree<HTML::Node> dom = parser.parseTree(html);
  tree <HTML::Node> :: iterator it = findScoreboard(dom);
  infocolumns cols;
  it=findProblems(it,dom.end(),cols);
  if (PROBLEM_FIRST_COLUMN!=-1) { /* Parche: hardcodeamos la primera columna de problemas */
	cols.lastProblem += PROBLEM_FIRST_COLUMN-cols.firstProblem;
	cols.firstProblem = PROBLEM_FIRST_COLUMN;
  }
  assert(it!=dom.end());
  fillScoreboard(it,dom.end(),cols,score);
}

void printScore(scoreboard& score) {
	forall(it,score.s) {
		cout<<it->name<<": ";
		forall(jt,it->problems){ if (jt->first!=-1 && jt->second<=global_time) cout << jt->first << " / " << jt->second;
								 else cout << "  X    ";
			cout << " ||| ";
		}
		cout << endl;
	}
}

void ActualizarCant(scoreboard& s, vector<int>& r) {
	r.clear();
	r.assign(s.s[0].problems.size(),0);
	forall(it,s.s) forn(i,sz(it->problems)) if (it->problems[i].first!=-1 && it->problems[i].second<=global_time) r[i]++;
}

int main (int argc, char *argv[]) {
  if (argc<3) {
	cout << "Forget the name of input.html output.html" << endl; 
	return 1;
  }
  PROBLEM_FIRST_COLUMN = (argc>3?atoi(argv[3]):-1);
  
  scoreboard score;
  ScoreMain(argv[1],score);
  
  for (global_time=0;global_time<=300;global_time+=1) { // ACA PUEDEN MODIFICAR CUANTO AVANZA EL TIEMPO POR REFRESH
	  sort(score.s.begin(),score.s.end());
	  
	  fstream ss (argv[2], fstream::out);
	  ss << "<h1> minute: " << global_time << "</h1>" << endl;
	  ss << "<table border=1>" << endl;
	  ss << "<tr>";
	  ss << "<th> Team </th>";
	  vector<int> cantSolved;
	  ActualizarCant(score,cantSolved);
	  for(int i = 0 ; i<sz(score.s[0].problems) ; i++) ss << " <th> Problem "<< char('A'+i)<<"<br>" << cantSolved[i] << "" <<"</th>";
	  ss << "</tr>\n";
	  for(int ei=0;ei<sz(score.s);ei++) {
		  ss << "<tr>";
		  ss << "<td>"<< score.s[ei].name <<"</td>";
		  for(int pi=0;pi<sz(score.s[ei].problems);pi++) {
			ss << " <td align=center>";
			if (score.s[ei].problems[pi].first!=-1 && score.s[ei].problems[pi].second<=global_time)
				ss << score.s[ei].problems[pi].first << " - " << score.s[ei].problems[pi].second ;
			ss<<"</td>";
		  }
		  ss << "</tr>\n";
	  }
	  ss << "</table>";
	  ss.close();
	  
	  sleep(1); // ACA PUEDEN MODIFICAR CUANTO DUERME ANTES DEL SIGUIENTE REFRESH
  }
  return 0;
}
