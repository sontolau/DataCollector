package sonto.models.fields;

import sonto.exception.InvalidValueTypeException;
import sonto.models.Model;

public class ForeignKey extends Field {
    public Class<Model> refModel = null;
    
    public ForeignKey(String name, Class<Model> model, FieldAttribute[] attrs){
        super(name, attrs);
        
        this.refModel = model;
        // TODO Auto-generated constructor stub
    }
    
    public void validate(Object v) throws InvalidValueTypeException {
        if (!(v instanceof Model)) {
            throw new InvalidValueTypeException ();
        }
    }
}
