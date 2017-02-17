from django.contrib import admin
from pyweb.mod_auth.models import *

# Register your models here.
admin.site.register(PermissionModel)
admin.site.register(RoleModel)
admin.site.register(UserModel)
