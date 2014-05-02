/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "TreeWorker.h"

#include "Logic/NodeTree.h"
#include "Logic/NodeFlowData.h"
#include "Logic/NodeException.h"

TreeWorker::TreeWorker(QObject* parent)
    : QObject(parent)
{
}

TreeWorker::~TreeWorker()
{
}

void TreeWorker::setNodeTree(const std::shared_ptr<NodeTree>& nodeTree)
{
    // Could use mutex here but we try really hard not to call it when process() is working
    _nodeTree = nodeTree;
}

void TreeWorker::process(bool withInit)
{
    bool res = false;
    try
    {
        if(_nodeTree)
            _nodeTree->execute(withInit);
        res = true;
    }
    catch(ExecutionError& ex)
    {
        emit error(QString("Execution error in:\nNode: %1\n"
            "Node typename: %2\n\nError message:\n%3")
                .arg(ex.nodeName.c_str())
                .arg(ex.nodeTypeName.c_str())
                .arg(ex.errorMessage.c_str()));
    }
    catch(...)
    {
        emit error(QStringLiteral("Internal error - Unknown exception was caught"));
    }

    emit completed(res);
}

