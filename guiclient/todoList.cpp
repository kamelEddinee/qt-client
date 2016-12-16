/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "todoList.h"
#include "xdialog.h"
#include "todoItem.h"
#include "incident.h"
#include "customer.h"
#include "project.h"
#include "opportunity.h"
#include "storedProcErrorLookup.h"
#include "task.h"
#include "parameterwidget.h"

#include <QMessageBox>
#include <QSqlError>
#include <QToolBar>
#include "errorReporter.h"

todoList::todoList(QWidget* parent, const char*, Qt::WindowFlags fl)
  : display(parent, "todoList", fl)
{
  _shown = false;
  _run = false;

  setupUi(optionsWidget());
  setWindowTitle(tr("To-Do Items"));
  setReportName("TodoList");
  setMetaSQLOptions("todolist", "detail");
  setUseAltId(true);
  setParameterWidgetVisible(true);
  setNewVisible(true);
  setQueryOnStartEnabled(true);

  parameterWidget()->append(tr("User"), "username", ParameterWidget::User, omfgThis->username());
  parameterWidget()->append(tr("Owner"), "owner_username", ParameterWidget::User);
  parameterWidget()->append(tr("Assigned To"), "assigned_username", ParameterWidget::User);
  parameterWidget()->append(tr("Account"), "crmacct_id", ParameterWidget::Crmacct);
  parameterWidget()->append(tr("Start Date on or Before"), "startStartDate", ParameterWidget::Date);
  parameterWidget()->append(tr("Start Date on or After"), "startEndDate", ParameterWidget::Date);
  parameterWidget()->append(tr("Due Date on or Before"), "dueStartDate", ParameterWidget::Date);
  parameterWidget()->append(tr("Due Date on or After"), "dueEndDate", ParameterWidget::Date);
  parameterWidget()->append(tr("Show Completed"), "completed", ParameterWidget::Exists);
  parameterWidget()->append(tr("Show Completed Only"), "completedonly", ParameterWidget::Exists);

  connect(_opportunities, SIGNAL(toggled(bool)), this, SLOT(sFillList()));
  connect(_todolist, SIGNAL(toggled(bool)), this,   SLOT(sFillList()));
  connect(_incidents, SIGNAL(toggled(bool)), this, SLOT(sFillList()));
  connect(_projects, SIGNAL(toggled(bool)), this,	SLOT(sFillList()));
  connect(list(), SIGNAL(itemSelected(int)), this, SLOT(sOpen()));

  list()->addColumn(tr("Type"),      _userColumn,  Qt::AlignCenter, true, "type");
  list()->addColumn(tr("Priority"),  _userColumn,  Qt::AlignLeft,   true, "priority");
  list()->addColumn(tr("Owner"),     _userColumn,  Qt::AlignLeft,   false,"owner");
  list()->addColumn(tr("Assigned To"),_userColumn, Qt::AlignLeft,   true, "assigned");
  list()->addColumn(tr("Name"),              100,  Qt::AlignLeft,   true, "name");
  list()->addColumn(tr("Notes"),        -1,  Qt::AlignLeft,   true, "notes");
  list()->addColumn(tr("Stage"),   _statusColumn,  Qt::AlignLeft,   true, "stage");
  list()->addColumn(tr("Start Date"),_dateColumn,  Qt::AlignLeft,   false, "start");
  list()->addColumn(tr("Due Date"),  _dateColumn,  Qt::AlignLeft,   true, "due");
  list()->addColumn(tr("Account#"), _orderColumn,  Qt::AlignLeft,   false, "crmacct_number");
  list()->addColumn(tr("Account Name"),      100,  Qt::AlignLeft,   true, "crmacct_name");
  list()->addColumn(tr("Parent"),            100,  Qt::AlignLeft,   false, "parent");
  list()->addColumn(tr("Customer"),    _ynColumn,  Qt::AlignLeft,   false, "cust");

  QToolButton * newBtn = (QToolButton*)toolBar()->widgetForAction(newAction());
  newBtn->setPopupMode(QToolButton::MenuButtonPopup);
  QAction *menuItem;
  QMenu * todoMenu = new QMenu;
  menuItem = todoMenu->addAction(tr("To-Do Item"),   this, SLOT(sNew()));
  if(todoItem::userHasPriv(cNew))
    menuItem->setShortcut(QKeySequence::New);
  menuItem->setEnabled(todoItem::userHasPriv(cNew));
  menuItem = todoMenu->addAction(tr("Opportunity"), this, SLOT(sNewOpportunity()));
  menuItem->setEnabled(opportunity::userHasPriv(cNew));
  menuItem = todoMenu->addAction(tr("Incident"), this, SLOT(sNewIncdt()));
  menuItem->setEnabled(incident::userHasPriv(cNew));
  menuItem = todoMenu->addAction(tr("Project"), this, SLOT(sNewProject()));
  menuItem->setEnabled(project::userHasPriv(cNew));
  newBtn->setMenu(todoMenu);
}

void todoList::showEvent(QShowEvent * event)
{
  display::showEvent(event);

  if(!_shown)
  {
    _shown = true;
    if(_run)
      sFillList();
  }
}

void todoList::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *, int)
{
  QAction *menuItem;

  if (list()->currentItem()->altId() == 1)
  {
    menuItem = pMenu->addAction(tr("Edit To-Do..."), this, SLOT(sEdit()));
    menuItem->setEnabled(todoItem::userHasPriv(cEdit, getId(1)));

    menuItem = pMenu->addAction(tr("View To-Do..."), this, SLOT(sView()));
    menuItem->setEnabled(todoItem::userHasPriv(cView, getId(1)));

    menuItem = pMenu->addAction(tr("Delete To-Do"), this, SLOT(sDelete()));
    menuItem->setEnabled(todoItem::userHasPriv(cEdit, getId(1)));
  }

  pMenu->addSeparator();

  if (list()->altId() == 2 ||
      (list()->currentItem()->altId() == 1 &&
       list()->currentItem()->rawValue("parent") == "INCDT"))
  {
    menuItem = pMenu->addAction(tr("Edit Incident"), this, SLOT(sEditIncident()));
    menuItem->setEnabled(incident::userHasPriv(cEdit, getId(2)));
    menuItem = pMenu->addAction(tr("View Incident"), this, SLOT(sViewIncident()));
    menuItem->setEnabled(incident::userHasPriv(cView, getId(2)));
  }
  pMenu->addSeparator();

  if (list()->altId() == 3)
  {
    menuItem = pMenu->addAction(tr("Edit Task"), this, SLOT(sEditTask()));
    menuItem->setEnabled(task::userHasPriv(cEdit, getId(3)));
    menuItem = pMenu->addAction(tr("View Task"), this, SLOT(sViewTask()));
    menuItem->setEnabled(task::userHasPriv(cView, getId(3)));
    pMenu->addSeparator();
  }

  if (list()->altId() == 3 || list()->altId() == 4)
  {
    menuItem = pMenu->addAction(tr("Edit Project"), this, SLOT(sEditProject()));
    menuItem->setEnabled(project::userHasPriv(cEdit, getId(4)));
    menuItem = pMenu->addAction(tr("View Project"), this, SLOT(sViewProject()));
    menuItem->setEnabled(project::userHasPriv(cView, getId(4)));
  }

  if (list()->altId() == 5  ||
      (list()->currentItem()->altId() == 1 &&
       list()->currentItem()->rawValue("parent") == "OPP"))
  {
    menuItem = pMenu->addAction(tr("Edit Opportunity"), this, SLOT(sEditOpportunity()));
    menuItem->setEnabled(opportunity::userHasPriv(cEdit, getId(5)));
    menuItem = pMenu->addAction(tr("View Opportunity"), this, SLOT(sViewOpportunity()));
    menuItem->setEnabled(opportunity::userHasPriv(cView, getId(5)));
  }

  if (list()->currentItem()->rawValue("cust").toInt() > 0)
  {
    pMenu->addSeparator();
    menuItem = pMenu->addAction(tr("Edit Customer"), this, SLOT(sEditCustomer()));
    menuItem->setEnabled(customer::userHasPriv(cEdit));
    menuItem = pMenu->addAction(tr("View Customer"), this, SLOT(sViewCustomer()));
    menuItem->setEnabled(customer::userHasPriv(cView));
  }
}

enum SetResponse todoList::set(const ParameterList& pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool	   valid;

  param = pParams.value("run", &valid);
  if (valid)
    sFillList();

  return NoError;
}

void todoList::sNew()
{
  //Need an extra priv check because of display trigger
  if (!todoItem::userHasPriv(cNew))
    return;

  ParameterList params;
  parameterWidget()->appendValue(params);
  params.append("mode", "new");

  todoItem newdlg(this, "", true);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sNewIncdt()
{
  ParameterList params;
  parameterWidget()->appendValue(params);
  params.append("mode", "new");

  incident newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sEdit()
{
  if (list()->altId() ==2)
    sEditIncident();
  else if (list()->altId() == 3)
    sEditTask();
  else if (list()->altId() == 4)
    sEditProject();
  else if (list()->altId() == 5)
    sEditOpportunity();
  else
  {
    ParameterList params;
    params.append("mode", "edit");
    params.append("todoitem_id", list()->id());

    todoItem newdlg(this, "", true);
    newdlg.set(params);

    if (newdlg.exec() != XDialog::Rejected)
      sFillList();
  }
}

void todoList::sView()
{
  if (list()->altId() ==2)
    sViewIncident();
  else if (list()->altId() == 3)
    sViewTask();
  else if (list()->altId() == 4)
    sViewProject();
  else if (list()->altId() == 5)
    sViewOpportunity();
  else
  {
    ParameterList params;
    params.append("mode", "view");
    params.append("todoitem_id", list()->id());

    todoItem newdlg(this, "", true);
    newdlg.set(params);

    newdlg.exec();
  }
}

void todoList::sDelete()
{
  XSqlQuery todoDelete;
  QString recurstr;
  QString recurtype;
  if (list()->altId() == 1)
  {
    recurstr = "SELECT MAX(todoitem_due_date) AS max"
               "  FROM todoitem"
               " WHERE todoitem_recurring_todoitem_id=:id"
               "   AND todoitem_id!=:id;" ;
    recurtype = "TODO";
  }

  bool deleteAll  = false;
  bool deleteOne  = false;
  if (! recurstr.isEmpty())
  {
    XSqlQuery recurq;
    recurq.prepare(recurstr);
    recurq.bindValue(":id", list()->id());
    recurq.exec();
    if (recurq.first() && !recurq.value("max").isNull())
    {
      QMessageBox askdelete(QMessageBox::Question, tr("Delete Recurring Item?"),
                            tr("<p>This is a recurring item. Do you want to "
                               "delete just this one item or delete all open "
                               "items in this recurrence?"),
                            QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel,
                            this);
      askdelete.setDefaultButton(QMessageBox::Cancel);
      int ret = askdelete.exec();
      if (ret == QMessageBox::Cancel)
        return;
      else if (ret == QMessageBox::YesToAll)
        deleteAll = true;
      // user said delete one but the only one that exists is the parent ToDo
      else if (ret == QMessageBox::Yes)
        deleteOne = true;
    }
    else if (ErrorReporter::error(QtCriticalMsg, this, tr("Error Retrieving To Do Item Information"),
                                  recurq, __FILE__, __LINE__))
    {
      return;
    }
    else if (QMessageBox::warning(this, tr("Delete List Item?"),
                                  tr("<p>Are you sure that you want to "
                                     "completely delete the selected item?"),
	  		    QMessageBox::Yes | QMessageBox::No,
			    QMessageBox::No) == QMessageBox::No)
      return;
  }
  else if (QMessageBox::warning(this, tr("Delete List Item?"),
                                tr("<p>Are you sure that you want to "
                                   "completely delete the selected item?"),
	  		    QMessageBox::Yes | QMessageBox::No,
			    QMessageBox::No) == QMessageBox::No)
    return;

  int procresult = 0;
  if (deleteAll)  // Delete all todos in the recurring series
  {
    todoDelete.prepare("SELECT deleteOpenRecurringItems(:id, :type, NULL, true)"
              "       AS result;");
    todoDelete.bindValue(":id",   list()->id());
    todoDelete.bindValue(":type", recurtype);
    todoDelete.exec();
    if (todoDelete.first())
      procresult = todoDelete.value("result").toInt();

    if (procresult < 0)
    {
      ErrorReporter::error(QtCriticalMsg, this, tr("Error Retrieving Recurring To Do Item Information"),
                             storedProcErrorLookup("deleteOpenRecurringItems", procresult),
                             __FILE__, __LINE__);
      return;
    }
    else if (ErrorReporter::error(QtCriticalMsg, this, tr("Error Retrieving Recurring To Do Item Information"),
                                  todoDelete, __FILE__, __LINE__))
    {
      return;
    }
  }

  if (deleteOne) // The base todo in a recurring series has been seleted.  Have to move
                 // recurrence to the next item else we hit foreign key errors.
                 // Make the next item on the list the parent in the series
  {
    todoDelete.prepare("UPDATE todoitem SET todoitem_recurring_todoitem_id =("
                        "               SELECT MIN(todoitem_id) FROM todoitem"
                        "                 WHERE todoitem_recurring_todoitem_id=:id"
                        "                   AND todoitem_id!=:id)"
                        "  WHERE todoitem_recurring_todoitem_id=:id"
                        "  AND todoitem_id!=:id;");
    todoDelete.bindValue(":id",   list()->id());
    todoDelete.exec();
    if (ErrorReporter::error(QtCriticalMsg, this, tr("Error Deleting Recurring To Do Item Information"),
                                  todoDelete, __FILE__, __LINE__))
    {
      return;
    }
  }

  if (list()->altId() == 1)
    todoDelete.prepare("SELECT deleteTodoItem(:todoitem_id) AS result;");
  else if (list()->altId() == 3)
    todoDelete.prepare("DELETE FROM prjtask"
              " WHERE (prjtask_id=:todoitem_id); ");
  else if (list()->altId() == 4)
    todoDelete.prepare("SELECT deleteProject(:todoitem_id) AS result");
  else
    return;
  todoDelete.bindValue(":todoitem_id", list()->id());
  todoDelete.exec();
  if (todoDelete.first())
  {
    int result = todoDelete.value("result").toInt();
    if (result < 0)
    {
      ErrorReporter::error(QtCriticalMsg, this, tr("Error Retrieving To Do Item Information"),
                             storedProcErrorLookup("deleteTodoItem", result),
                             __FILE__, __LINE__);
      return;
    }
  }
  else if (ErrorReporter::error(QtCriticalMsg, this, tr("Error Retrieving To Do Item Information"),
                                todoDelete, __FILE__, __LINE__))
  {
     return;
  }
  sFillList();
}

bool todoList::setParams(ParameterList &params)
{
  if (!_todolist->isChecked() &&
      !_opportunities->isChecked() &&
      !_incidents->isChecked() &&
      !_projects->isChecked())
  {
    list()->clear();
    return false;
  }

  if (_todolist->isChecked())
    params.append("todoList");
  if (_opportunities->isChecked())
    params.append("opportunities");
  if (_incidents->isChecked())
    params.append("incidents");
  if (_projects->isChecked())
    params.append("projects");

  params.append("todo", tr("To-do"));
  params.append("incident", tr("Incident"));
  params.append("task", tr("Task"));
  params.append("project", tr("Project"));
  params.append("opportunity", tr("Opportunity"));
  params.append("complete", tr("Completed"));
  params.append("deferred", tr("Deferred"));
  params.append("pending", tr("Pending"));
  params.append("inprocess", tr("InProcess"));
  params.append("feedback", tr("Feedback"));
  params.append("confirmed", tr("Confirmed"));
  params.append("assigned", tr("Assigned"));
  params.append("resolved", tr("Resolved"));
  params.append("closed", tr("Closed"));
  params.append("concept", tr("Concept"));
  params.append("new", tr("New"));

  if (!display::setParams(params))
    return false;

  return true;
}

int todoList::getId(int pType)
{
  if (list()->currentItem()->altId() == pType)
    return list()->id();
  else
    return list()->currentItem()->id("parent");
}

void todoList::sEditIncident()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("incdt_id", getId(2));

  incident newdlg(this, "", true);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sViewIncident()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("incdt_id", getId(2));

  incident newdlg(this, "", true);
  newdlg.set(params);

  newdlg.exec();
}

void todoList::sNewProject()
{
  ParameterList params;
  parameterWidget()->appendValue(params);
  params.append("mode", "new");

  project newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sEditProject()
{
  ParameterList params;
  params.append("mode", "edit");
  qDebug("project %d", getId(4));
  params.append("prj_id", getId(4));

  project newdlg(this, "", true);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sViewProject()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("prj_id", getId(4));

  project newdlg(this, "", true);
  newdlg.set(params);

  newdlg.exec();
}

void todoList::sEditTask()
{

  ParameterList params;
  params.append("mode", "edit");
  params.append("prjtask_id", list()->id());

  task newdlg(this, "", true);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sViewTask()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("prjtask_id", list()->id());

  task newdlg(this, "", true);
  newdlg.set(params);

  newdlg.exec();
}

void todoList::sEditCustomer()
{
  ParameterList params;
  params.append("cust_id", list()->rawValue("cust").toInt());
  params.append("mode","edit");

  customer *newdlg = new customer(this);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void todoList::sViewCustomer()
{
  ParameterList params;
  params.append("cust_id", list()->rawValue("cust").toInt());
  params.append("mode","view");

  customer *newdlg = new customer(this);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void todoList::sNewOpportunity()
{
  ParameterList params;
  parameterWidget()->appendValue(params);
  params.append("mode", "new");

  opportunity newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sEditOpportunity()
{

  ParameterList params;
  params.append("mode", "edit");
  params.append("ophead_id", getId(5));

  opportunity newdlg(this, "", true);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void todoList::sViewOpportunity()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("ophead_id", getId(5));

  opportunity newdlg(this, "", true);
  newdlg.set(params);

  newdlg.exec();
}

void todoList::sOpen()
{
  bool editPriv = false;
  bool viewPriv = false;

    switch (list()->altId())
    {
    case 1:
      editPriv = todoItem::userHasPriv(cEdit, list()->currentItem()->id());
      viewPriv = todoItem::userHasPriv(cView, list()->currentItem()->id());
      break;
    case 2:
      editPriv = incident::userHasPriv(cEdit, list()->currentItem()->id());
      viewPriv = incident::userHasPriv(cView, list()->currentItem()->id());
      break;
    case 3:
      editPriv = task::userHasPriv(cEdit, list()->currentItem()->id());
      viewPriv = task::userHasPriv(cView, list()->currentItem()->id());
      break;
    case 4:
      editPriv = project::userHasPriv(cEdit, list()->currentItem()->id());
      viewPriv = project::userHasPriv(cView, list()->currentItem()->id());
      break;
    case 5:
      editPriv = opportunity::userHasPriv(cEdit, list()->currentItem()->id());
      viewPriv = opportunity::userHasPriv(cView, list()->currentItem()->id());
      break;
    default:
      break;
    }

  if(editPriv)
    sEdit();
  else if(viewPriv)
    sView();
  else
    QMessageBox::information(this, tr("Restricted Access"), tr("You have not been granted privileges to open this item."));
}

void todoList::sFillList()
{
  if(_shown)
    display::sFillList();
  else
    _run = true;
}



