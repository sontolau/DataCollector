# coding=utf-8
from __future__ import unicode_literals

# Create your models here.

# coding=utf-8
from django.contrib.auth.base_user import AbstractBaseUser, BaseUserManager
from django.db import models

__all__ = ['PermissionModel', 'RoleModel', 'UserModel', 'DefaultUserManager']


class PermissionModel(models.Model):
    code = models.CharField(max_length=51, unique=True, primary_key=True, verbose_name=u'权限代码')
    name = models.CharField(max_length=255, verbose_name=u'权限名称')
    create_time = models.DateTimeField(auto_now=True, verbose_name=u'创建时间')

    class Meta:
        db_table = 't_Permission'
        verbose_name = u'权限管理'

    def __unicode__(self):
        return self.code

    def natural_key(self):
        return (self.code,)


class RoleModel(models.Model):
    role = models.CharField(max_length=50, unique=True, primary_key=True, verbose_name=u'角色')
    create_time = models.DateTimeField(auto_now=True, verbose_name=u'创建时间')
    description = models.CharField(max_length=1024, blank=True, verbose_name=u'角色描述')
    is_superuser = models.BooleanField(default=False, verbose_name=u'管理员?')
    permissions = models.ManyToManyField(PermissionModel, blank=True, verbose_name=u'权限集')

    class Meta:
        db_table = 't_Role'
        verbose_name = u'角色管理'

    def natural_key(self):
        return self.role,

    def __unicode__(self):
        return self.role


class DefaultUserManager(BaseUserManager):
    def create_user(self, username, email, password, role):
        user = self.model(email=self.normalize_email(email),
                          username=username,
                          role=role)
        user.set_password(password)
        user.save(force_insert=True)
        return user

    def create_superuser(self, username, password, **kwargs):
        role, created = RoleModel.objects.get_or_create(role="root",
                                                        is_superuser=True)
        if created:
            role.description = "root"
            role.save(force_update=True)

        user = self.model(username=username,
                          role=role)
        user.set_password(password)
        for k in kwargs.keys():
            setattr(user, k, kwargs[k])

        user.save(using=self._db)
        return user

    def get_by_natural_key(self, username):
        return self.get(username=username)


class UserModel(AbstractBaseUser):
    def get_full_name(self):
        return self.username

    def get_short_name(self):
        return self.username

    def has_perm(self, perm, obj=None):
        if self.role.is_superuser:
            return True

        for p in self.role.permissions.all():
            if p.code == perm:
                return True

        return False

    def has_perms(self, perms):
        if self.role.is_superuser:
            return True

        for p in perms:
            if not self.has_perm(p):
                return False

        return True

    def has_module_perms(self, app_label):
        return True

    @property
    def is_staff(self):
        return True

    @property
    def is_admin(self):
        return self.role.is_superuser

    username = models.CharField(max_length=50, unique=True, primary_key=True, verbose_name=u'帐户')
    email = models.EmailField(max_length=255, verbose_name=u'邮件')
    role = models.ForeignKey(RoleModel, verbose_name=u'角色')
    create_time = models.DateTimeField(auto_now=True, verbose_name=u'创建时间')
    USERNAME_FIELD = 'username'
    # REQUIRED_FIELDS = ['email']
    objects = DefaultUserManager()

    def __unicode__(self):
        return self.username

    def natural_key(self):
        return self.username

    class Meta:
        db_table = 't_User'
        verbose_name = u'用户管理'