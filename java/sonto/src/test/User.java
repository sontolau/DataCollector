package test;

import sonto.common.KeyValue;
import sonto.models.Model;
import sonto.models.ModelManager;
import sonto.models.fields.CharField;
import sonto.models.fields.Field;

public class User extends Model {
	public User(KeyValue[] kvs) throws Exception {
        super(kvs);
        // TODO Auto-generated constructor stub
    }

	/* set table name */
    public static String db_table = "wb_user";
    public static Field[] db_fields = new Field[] {new CharField("username", 255, null),
        new CharField("password", 50, null)};
	
	public static void main(String[] argv) throws Exception {
		ModelManager.registerModel(User.class);
		User.dumpFields();
	}
}
