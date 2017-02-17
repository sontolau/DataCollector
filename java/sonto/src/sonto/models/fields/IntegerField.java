package sonto.models.fields;

import sonto.exception.InvalidValueTypeException;

public class IntegerField extends Field {

    public IntegerField(String name, FieldAttribute[] attrs) {
        super(name, attrs);
        // TODO Auto-generated constructor stub
    }

    public void validate(Object v) throws InvalidValueTypeException {
        if (!(v instanceof Integer)) {
            throw new InvalidValueTypeException ();
        }
    }
}
