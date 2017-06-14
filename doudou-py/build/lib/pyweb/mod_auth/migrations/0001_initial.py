# -*- coding: utf-8 -*-
# Generated by Django 1.10.5 on 2017-02-18 02:44
from __future__ import unicode_literals

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='UserModel',
            fields=[
                ('password', models.CharField(max_length=128, verbose_name='password')),
                ('last_login', models.DateTimeField(blank=True, null=True, verbose_name='last login')),
                ('username', models.CharField(max_length=50, primary_key=True, serialize=False, unique=True, verbose_name='\u5e10\u6237')),
                ('email', models.EmailField(max_length=255, verbose_name='\u90ae\u4ef6')),
                ('create_time', models.DateTimeField(auto_now=True, verbose_name='\u521b\u5efa\u65f6\u95f4')),
            ],
            options={
                'db_table': 't_User',
                'verbose_name': '\u7528\u6237\u7ba1\u7406',
            },
        ),
        migrations.CreateModel(
            name='PermissionModel',
            fields=[
                ('code', models.CharField(max_length=51, primary_key=True, serialize=False, unique=True, verbose_name='\u6743\u9650\u4ee3\u7801')),
                ('name', models.CharField(max_length=255, verbose_name='\u6743\u9650\u540d\u79f0')),
                ('create_time', models.DateTimeField(auto_now=True, verbose_name='\u521b\u5efa\u65f6\u95f4')),
            ],
            options={
                'db_table': 't_Permission',
                'verbose_name': '\u6743\u9650\u7ba1\u7406',
            },
        ),
        migrations.CreateModel(
            name='RoleModel',
            fields=[
                ('role', models.CharField(max_length=50, primary_key=True, serialize=False, unique=True, verbose_name='\u89d2\u8272')),
                ('create_time', models.DateTimeField(auto_now=True, verbose_name='\u521b\u5efa\u65f6\u95f4')),
                ('description', models.CharField(blank=True, max_length=1024, verbose_name='\u89d2\u8272\u63cf\u8ff0')),
                ('is_superuser', models.BooleanField(default=False, verbose_name='\u7ba1\u7406\u5458?')),
                ('permissions', models.ManyToManyField(blank=True, to='mod_auth.PermissionModel', verbose_name='\u6743\u9650\u96c6')),
            ],
            options={
                'db_table': 't_Role',
                'verbose_name': '\u89d2\u8272\u7ba1\u7406',
            },
        ),
        migrations.AddField(
            model_name='usermodel',
            name='role',
            field=models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='mod_auth.RoleModel', verbose_name='\u89d2\u8272'),
        ),
    ]
