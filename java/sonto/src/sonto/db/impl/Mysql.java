package sonto.db.impl;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Map;

import sonto.db.DBIface;
import sonto.models.Model;
import sonto.models.fields.AutoField;
import sonto.models.fields.BooleanField;
import sonto.models.fields.CharField;
import sonto.models.fields.Field;
import sonto.models.fields.FloatField;
import sonto.models.fields.IntegerField;

public class Mysql extends DBIface {
    public Mysql(String host, int port, String name, String user,
            String password) {
        super(host, port, name, user, password);
        // TODO Auto-generated constructor stub
    }

    private Connection mysqlConn = null;

    @Override
    public void disconnect() throws Exception {
        // TODO Auto-generated method stub

        this.mysqlConn.close();

    }

    @Override
    public boolean connect(String host, int port, String name, String user,
            String password) throws Exception {
        Class.forName("com.mysql.jdbc.Driver").newInstance();
        this.mysqlConn = DriverManager.getConnection("jdbc:mysql://" + host
                + ":" + port + "/name", user, password);
        return true;
    }

    @Override
    public void update(Model model) {
    }

    @Override
    public void insert(Model model) {
        // TODO Auto-generated method stub
        
    }

    @Override
    public void create_table(Class<Model> model, Map<String, String> options)
            throws Exception {
        // TODO Auto-generated method stub
        
    }
}
