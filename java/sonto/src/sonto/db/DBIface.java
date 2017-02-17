package sonto.db;

import java.util.Map;

import sonto.models.Model;

public abstract class DBIface {
    public String host;
    public int port;
    public String name;
    public String user;
    public String password;
    
    public DBIface (String host, int port, String name, String user, String password) {
        this.host = host;
        this.port = port;
        this.name = name;
        this.user = user;
        this.password = password;
    }
    
	abstract public boolean connect(String host, int port, String name, String user, String password) throws Exception ;
	abstract public void disconnect () throws Exception;
	abstract public void create_table(Class<Model> model, Map<String, String> options) throws Exception;
	abstract public void update(Model model);
	abstract public void insert (Model model);
	abstract public void delete (Model model);
}
